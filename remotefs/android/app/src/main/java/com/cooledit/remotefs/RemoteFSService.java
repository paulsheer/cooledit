package com.cooledit.remotefs;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.net.wifi.WifiManager;
import android.os.Binder;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.PowerManager;

/**
 * Foreground service that keeps the remotefs server alive
 * when the app is backgrounded.
 */
public class RemoteFSService extends Service {

    private static final String CHANNEL_ID = "remotefs_channel";
    private static final int NOTIFICATION_ID = 1;

    private static PowerManager.WakeLock wakeLock;
    private static WifiManager.WifiLock wifiLock;
    private static Handler wifiLockHandler;
    private static Runnable wifiLockReleaser;
    private Thread serverThread;
    private boolean isRunning = false;

    /* Callback from native code to control log window polling */
    public interface PollingListener {
        void onEnablePolling(boolean enable);
    }

    private static PollingListener pollingListener;

    public static void setPollingListener(PollingListener listener) {
        pollingListener = listener;
    }

    /* Native methods implemented in android.c */
    private static native boolean nativeStart(String listenAddr, String ipRange, String keyfilePath);
    private static native void nativeStop();
    private static native boolean nativeIsRunning();
    private static native void nativeCreateAESKey(String keyfilePath);
    private static native void nativeInitLogWindow(java.nio.ByteBuffer buffer);
    private static native void nativeClearLogWindow();

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
            stopServer(startId);
        } else if ("update_notification".equals(action)) {
            updateForegroundNotification();
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

        /* Acquire Wi-Fi lock to keep radio at full power (mirrors wake lock 10-min timeout) */
        WifiManager wm = (WifiManager) getApplicationContext().getSystemService(Context.WIFI_SERVICE);
        if (wm != null) {
            wifiLock = wm.createWifiLock(WifiManager.WIFI_MODE_FULL, "RemoteFS::WiFiLock");
            wifiLock.acquire();
            if (wifiLockHandler == null) {
                wifiLockHandler = new Handler(Looper.getMainLooper());
                wifiLockReleaser = new Runnable() {
                    public void run() {
                        if (wifiLock != null && wifiLock.isHeld())
                            wifiLock.release();
                    }
                };
            }
            wifiLockHandler.removeCallbacks(wifiLockReleaser);
            wifiLockHandler.postDelayed(wifiLockReleaser, 600000);
        }

        /* Start native server on a background thread */
        serverThread = new Thread(new Runnable() {
            @Override
            public void run() {
                boolean ok = nativeStart(finalListen, finalRange, finalKeyfile);
                if (!ok) {
                    new SettingsStore(RemoteFSService.this).setServerRunning(false);
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

        /* Show foreground notification only when enabled */
        if (new SettingsStore(this).getShowNotification()) {
            Notification notification = buildNotification("RemoteFS Server", "RemoteFS server is running");
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.UPSIDE_DOWN_CAKE) {
                startForeground(NOTIFICATION_ID, notification,
                    ServiceInfo.FOREGROUND_SERVICE_TYPE_SPECIAL_USE);
            } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                startForeground(NOTIFICATION_ID, notification,
                    ServiceInfo.FOREGROUND_SERVICE_TYPE_DATA_SYNC);
            } else {
                startForeground(NOTIFICATION_ID, notification);
            }
        }
    }

    /** Called from native code when client list empty state changes */
    public static void enablePolling(boolean enable) {
        if (pollingListener != null) {
            pollingListener.onEnablePolling(enable);
        }
    }

    /** Called from native code to refresh both locks on each client action */
    public static void refreshWakeLock() {
        if (wakeLock != null) {
            if (wakeLock.isHeld())
                wakeLock.release();
            wakeLock.acquire(600000);
        }
        if (wifiLock != null && wifiLockReleaser != null) {
            if (!wifiLock.isHeld())
                wifiLock.acquire();
            wifiLockHandler.removeCallbacks(wifiLockReleaser);
            wifiLockHandler.postDelayed(wifiLockReleaser, 600000);
        }
    }

    private void stopServer(int startId) {
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
        if (wifiLock != null) {
            if (wifiLockReleaser != null)
                wifiLockHandler.removeCallbacks(wifiLockReleaser);
            if (wifiLock.isHeld())
                wifiLock.release();
            wifiLock = null;
        }

        stopForeground(true);
        if (startId >= 0) {
            stopSelf(startId);
        }
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
               .setSmallIcon(R.drawable.ic_notification)
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

    public class LocalBinder extends Binder {
        public RemoteFSService getService() {
            return RemoteFSService.this;
        }
    }

    @Override
    public IBinder onBind(Intent intent) {
        return new LocalBinder();
    }

    /** Show or hide the foreground notification based on current setting */
    public void updateForegroundNotification() {
        if (!isRunning) return;

        if (new SettingsStore(this).getShowNotification()) {
            Notification notification = buildNotification("RemoteFS Server", "RemoteFS server is running");
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.UPSIDE_DOWN_CAKE) {
                startForeground(NOTIFICATION_ID, notification,
                    ServiceInfo.FOREGROUND_SERVICE_TYPE_SPECIAL_USE);
            } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                startForeground(NOTIFICATION_ID, notification,
                    ServiceInfo.FOREGROUND_SERVICE_TYPE_DATA_SYNC);
            } else {
                startForeground(NOTIFICATION_ID, notification);
            }
        } else {
            stopForeground(true);
        }
    }

    private static java.nio.ByteBuffer logWindowBuffer;

    /** Initialize the shared log window buffer from Java side */
    public static void initLogWindow(java.nio.ByteBuffer buffer) {
        logWindowBuffer = buffer;
        nativeInitLogWindow(buffer);
    }

    /** Clear the log window native pointer when the activity is destroyed */
    public static void clearLogWindow() {
        logWindowBuffer = null;
        nativeClearLogWindow();
    }

    /** Create AES keyfile from Java side before starting server */
    public static void createAESKey(String keyfilePath) {
        nativeCreateAESKey(keyfilePath);
    }

    @Override
    public void onDestroy() {
        stopServer(-1);
        super.onDestroy();
    }
}
