package com.cooledit.remotefs;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.os.Build;
import android.os.IBinder;
import android.os.PowerManager;

/**
 * Foreground service that keeps the remotefs server alive
 * when the app is backgrounded.
 */
public class RemoteFSService extends Service {

    private static final String CHANNEL_ID = "remotefs_channel";
    private static final int NOTIFICATION_ID = 1;

    private static PowerManager.WakeLock wakeLock;
    private Thread serverThread;
    private boolean isRunning = false;

    /* Native methods implemented in android-bridge.c */
    private static native boolean nativeStart(String listenAddr, String ipRange, String keyfilePath);
    private static native void nativeStop();
    private static native boolean nativeIsRunning();
    private static native void nativeCreateAESKey(String keyfilePath);

    static {
        System.loadLibrary("remotefs");
    }

    @Override
    public void onCreate() {
        super.onCreate();
        createNotificationChannel();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (intent == null) {
            /* System restart after kill/update — restore server if it was running */
            SettingsStore settings = new SettingsStore(this);
            if (settings.isServerRunning()) {
                Intent restartIntent = new Intent(this, RemoteFSService.class);
                restartIntent.putExtra("action", "start");
                restartIntent.putExtra("listen_addr", settings.getListenAddress());
                restartIntent.putExtra("ip_range", settings.getIpRange());
                restartIntent.putExtra("keyfile_path", settings.getKeyfilePath());
                startServer(restartIntent);
            }
            return START_STICKY;
        }

        String action = intent.getStringExtra("action");
        if ("start".equals(action)) {
            startServer(intent);
        } else if ("stop".equals(action)) {
            stopServer();
        }

        return START_STICKY;
    }

    private void startServer(Intent intent) {
        if (isRunning) {
            return;
        }

        String listenAddr = intent.getStringExtra("listen_addr");
        String ipRange = intent.getStringExtra("ip_range");
        String keyfilePath = intent.getStringExtra("keyfile_path");

        if (listenAddr == null) listenAddr = "0.0.0.0";
        if (ipRange == null) ipRange = "192.168.0.0/16";
        if (keyfilePath == null || keyfilePath.isEmpty())
            keyfilePath = getFilesDir().getAbsolutePath() + "/aeskeyfile";

        final String finalListen = listenAddr;
        final String finalRange = ipRange;
        final String finalKeyfile = keyfilePath;

        /* Acquire wake lock to keep CPU running (timed: auto-releases after 10 min idle) */
        PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        if (pm != null) {
            wakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "RemoteFS::WakeLock");
            wakeLock.acquire(600000);
        }

        /* Start native server on a background thread */
        serverThread = new Thread(new Runnable() {
            @Override
            public void run() {
                boolean ok = nativeStart(finalListen, finalRange, finalKeyfile);
                if (!ok) {
                    isRunning = false;
                    stopForeground(true);
                    stopSelf();
                }
            }
        });
        serverThread.setName("RemoteFS-Server");
        serverThread.setDaemon(false);
        serverThread.start();

        isRunning = true;

        /* Show foreground notification */
        Notification notification = buildNotification("RemoteFS Server", "RemoteFS server is running");
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            startForeground(NOTIFICATION_ID, notification,
                ServiceInfo.FOREGROUND_SERVICE_TYPE_DATA_SYNC);
        } else {
            startForeground(NOTIFICATION_ID, notification);
        }
    }

    /** Called from native code to refresh the wake lock on each client action */
    public static void refreshWakeLock() {
        if (wakeLock != null) {
            if (wakeLock.isHeld())
                wakeLock.release();
            wakeLock.acquire(600000);
        }
    }

    private void stopServer() {
        boolean serverWasStarted = (serverThread != null);

        if (serverWasStarted) {
            nativeStop();
            serverThread = null;
        }

        isRunning = false;

        if (wakeLock != null && wakeLock.isHeld()) {
            wakeLock.release();
            wakeLock = null;
        }

        stopForeground(true);
        stopSelf();
    }

    private Notification buildNotification(String title, String text) {
        Intent notificationIntent = new Intent(this, MainActivity.class);
        PendingIntent pendingIntent = PendingIntent.getActivity(
            this, 0, notificationIntent,
            PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE
        );

        Notification.Builder builder;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            builder = new Notification.Builder(this, CHANNEL_ID);
        } else {
            builder = new Notification.Builder(this);
        }

        builder.setContentTitle(title)
               .setContentText(text)
               .setSmallIcon(android.R.drawable.ic_menu_manage)
               .setOngoing(true)
               .setContentIntent(pendingIntent);

        return builder.build();
    }

    private void createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationManager manager = getSystemService(NotificationManager.class);
            if (manager != null) {
                /* Delete any pre-existing channel so importance/settings take effect */
                manager.deleteNotificationChannel(CHANNEL_ID);
                NotificationChannel channel = new NotificationChannel(
                    CHANNEL_ID,
                    "RemoteFS Service",
                    NotificationManager.IMPORTANCE_DEFAULT
                );
                channel.setDescription("Notification for RemoteFS server service");
                manager.createNotificationChannel(channel);
            }
        }
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    /** Create AES keyfile from Java side before starting server */
    public static void createAESKey(String keyfilePath) {
        nativeCreateAESKey(keyfilePath);
    }

    @Override
    public void onDestroy() {
        stopServer();
        super.onDestroy();
    }
}
