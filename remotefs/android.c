/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* android-bridge.c - JNI bridge for Android RemoteFS server
   Copyright (C) 1996-2022 Paul Sheer
 */

#include <jni.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <android/log.h>
#include "log_window.h"

#define LOG_TAG "RemoteFS-JNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

extern char *option_listen_address;
extern char *option_ip_range;
extern char *option_keyfile_path;
extern unsigned char the_key[];

int android_server_running = 0;

static JavaVM *cached_jvm;
static jobject service_obj;
static jmethodID refresh_wakelock_method;
static jmethodID enable_polling_method;

struct log_window_shared_data_s *volatile log_window = NULL;

JNIEXPORT jint JNICALL JNI_OnLoad (JavaVM *vm, void *reserved)
{
    (void) reserved;
    cached_jvm = vm;
    return JNI_VERSION_1_6;
}

/* Called from remotefs.c process_client() to keep the device awake */
void android_signal_activity (void)
{
    JNIEnv *env;
    int attached = 0;

    if (!cached_jvm || !service_obj || !refresh_wakelock_method)
        return;

    if ((*cached_jvm)->GetEnv (cached_jvm, (void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        if ((*cached_jvm)->AttachCurrentThread (cached_jvm, &env, NULL) != JNI_OK)
            return;
        attached = 1;
    }

    (*env)->CallStaticVoidMethod (env, (jclass) service_obj, refresh_wakelock_method);
    if ((*env)->ExceptionCheck (env)) {
        (*env)->ExceptionClear (env);
    }

    if (attached)
        (*cached_jvm)->DetachCurrentThread (cached_jvm);
}

void notify_java_enable_polling (int enable)
{
    JNIEnv *env;
    int attached = 0;

    if (!cached_jvm || !service_obj || !enable_polling_method)
        return;

    if ((*cached_jvm)->GetEnv (cached_jvm, (void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        if ((*cached_jvm)->AttachCurrentThread (cached_jvm, &env, NULL) != JNI_OK)
            return;
        attached = 1;
    }

    (*env)->CallStaticVoidMethod (env, (jclass) service_obj, enable_polling_method,
                                  enable ? JNI_TRUE : JNI_FALSE);
    if ((*env)->ExceptionCheck (env))
        (*env)->ExceptionClear (env);

    if (attached)
        (*cached_jvm)->DetachCurrentThread (cached_jvm);
}

static pthread_t server_thread;
static int server_thread_started = 0;

/* Declarations from remotefs.c */
void remotefs_serverize (void);
void remotefs_create_aes_key (const char *n);
void remotefs_read_keyfile (const char *n);
void remotefs_init_random (void);
void remotefs_set_kill_received (int v);

extern int init_random_done;

static void *server_thread_func (void *arg)
{
    (void) arg;
    LOGI ("Server thread starting...");
    remotefs_serverize ();
    LOGI ("Server thread exiting");
    init_random_done = 0;
    android_server_running = 0;
    return NULL;
}

JNIEXPORT jboolean JNICALL
Java_com_cooledit_remotefs_RemoteFSService_nativeStart (JNIEnv * env, jobject thiz,
                                                         jstring listenAddr,
                                                         jstring ipRange,
                                                         jstring keyfilePath)
{
    const char *listen_str;
    const char *range_str;
    const char *keyfile_str;

    (void) thiz;

    /* nativeStart is static so thiz is a jclass, not a jobject.
       Store it as a global ref for android_signal_activity() up-calls */
    {
        jclass cls = (jclass) thiz;
        if (service_obj) {
            (*env)->DeleteGlobalRef (env, service_obj);
            service_obj = NULL;
            refresh_wakelock_method = NULL;
        }
        service_obj = (*env)->NewGlobalRef (env, thiz);
        refresh_wakelock_method = (*env)->GetStaticMethodID (env, cls,
            "refreshWakeLock", "()V");
        if ((*env)->ExceptionCheck (env)) {
            (*env)->ExceptionClear (env);
            refresh_wakelock_method = NULL;
        }
        enable_polling_method = (*env)->GetStaticMethodID (env, cls,
            "enablePolling", "(Z)V");
        if ((*env)->ExceptionCheck (env)) {
            (*env)->ExceptionClear (env);
            enable_polling_method = NULL;
        }
    }

    if (server_thread_started && android_server_running) {
        LOGI ("Server already running");
        return JNI_TRUE;
    }

    if (server_thread_started) {
        LOGI ("Waiting for previous server thread to finish");
        pthread_join (server_thread, NULL);
        server_thread_started = 0;
    }

    /* Set listen address */
    listen_str = (*env)->GetStringUTFChars (env, listenAddr, NULL);
    if (option_listen_address) {
        /* Will be overwritten by remotefs_serverize on Android to "0.0.0.0" */
    }
    (*env)->ReleaseStringUTFChars (env, listenAddr, listen_str);

    /* Set IP range from Java config */
    free (option_ip_range);
    range_str = (*env)->GetStringUTFChars (env, ipRange, NULL);
    option_ip_range = strdup (range_str);
    (*env)->ReleaseStringUTFChars (env, ipRange, range_str);
    LOGI ("IP range set to: %s", option_ip_range);

    /* Set keyfile path */
    free (option_keyfile_path);
    keyfile_str = (*env)->GetStringUTFChars (env, keyfilePath, NULL);
    if (keyfile_str && keyfile_str[0]) {
        option_keyfile_path = strdup (keyfile_str);
    } else {
        option_keyfile_path = strdup ("AESKEYFILE");
    }
    (*env)->ReleaseStringUTFChars (env, keyfilePath, keyfile_str);
    LOGI ("Keyfile path: %s", option_keyfile_path);

    /* Initialize random number generator */
    remotefs_init_random ();

    {
        FILE *f;
        f = fopen (option_keyfile_path, "rb");
        if (f) {
            fclose (f);
        } else if (!f && errno == ENOENT) {
            /* ok, doesn't exist yet*/
            LOGE ("creating keyfile %s\n", option_keyfile_path);
            remotefs_create_aes_key (option_keyfile_path);
        } else if (!f) {
            LOGE ("accessing keyfile %s\n", option_keyfile_path);
            return JNI_FALSE;
        }
    }

    remotefs_read_keyfile (option_keyfile_path);

    remotefs_set_kill_received (0);
    android_server_running = 1;

    /* Start server in a background thread */
    if (pthread_create (&server_thread, NULL, server_thread_func, NULL) != 0) {
        LOGE ("Failed to create server thread");
        android_server_running = 0;
        return JNI_FALSE;
    }
    server_thread_started = 1;

    LOGI ("Server started successfully");
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_cooledit_remotefs_RemoteFSService_nativeStop (JNIEnv * env, jobject thiz)
{
    (void) thiz;

    if (!server_thread_started) {
        LOGI ("Server not running");
        return;
    }

    LOGI ("Stopping server...");
    remotefs_set_kill_received (1);
    android_server_running = 0;

    pthread_join (server_thread, NULL);
    server_thread_started = 0;

    asm volatile("": : :"memory");

    /* Clear log window so the next start or activity recreation re-initializes it */
    log_window = NULL;

    asm volatile("": : :"memory");

    LOGI ("Server stopped");
}

JNIEXPORT void JNICALL
Java_com_cooledit_remotefs_RemoteFSService_nativeClearLogWindow (JNIEnv * env, jclass clazz)
{
    (void) env;
    (void) clazz;
    log_window = NULL;
}

JNIEXPORT void JNICALL
Java_com_cooledit_remotefs_RemoteFSService_nativeCreateAESKey (JNIEnv * env, jclass clazz,
                                                                jstring keyfilePath)
{
    const char *path_str;

    (void) clazz;

    path_str = (*env)->GetStringUTFChars (env, keyfilePath, NULL);
    remotefs_create_aes_key (path_str);
    (*env)->ReleaseStringUTFChars (env, keyfilePath, path_str);
}

JNIEXPORT jboolean JNICALL
Java_com_cooledit_remotefs_RemoteFSService_nativeIsRunning (JNIEnv * env, jobject thiz)
{
    (void) env;
    (void) thiz;
    return android_server_running ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_cooledit_remotefs_RemoteFSService_nativeInitLogWindow (JNIEnv * env, jclass clazz,
                                                                 jobject buffer)
{
    jlong capacity;
    uint32_t rows, columns;
    struct log_window_shared_data_s *lw;

    (void) clazz;

    lw = (struct log_window_shared_data_s *)
        (*env)->GetDirectBufferAddress (env, buffer);
    if (!lw)
        return;

    capacity = (*env)->GetDirectBufferCapacity (env, buffer);
    rows = lw->lw_rows;
    columns = lw->lw_columns;

    /* Only touch memory we know is allocated */
    if ((jlong)(20 + rows * columns) > capacity)
        return;

    lw->lw_epoch = 0;
    lw->lw_current = 0;
    memset (lw->lw_data, ' ', rows * columns);

    asm volatile("": : :"memory");

    log_window = lw;

    asm volatile("": : :"memory");
}
