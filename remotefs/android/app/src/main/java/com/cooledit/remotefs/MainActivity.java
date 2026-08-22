package com.cooledit.remotefs;

import android.Manifest;
import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.net.ConnectivityManager;
import android.net.LinkAddress;
import android.net.LinkProperties;
import android.net.Network;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.PowerManager;
import android.provider.Settings;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TextView;
import android.util.Log;
import android.widget.Toast;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import com.google.zxing.BarcodeFormat;
import com.google.zxing.EncodeHintType;
import com.google.zxing.qrcode.QRCodeWriter;
import com.google.zxing.qrcode.decoder.ErrorCorrectionLevel;
import com.google.zxing.common.BitMatrix;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;

/**
 * Main activity with settings UI for the RemoteFS server.
 * Allows configuring the IP range and starting/stopping the server.
 */
public class MainActivity extends Activity {

    private EditText ipRangeEdit;
    private Button startButton;
    private Button stopButton;
    private TextView statusText;
    private TextView portInfo;
    private TextView qrLabel;
    private ImageView qrCode;
    private TextView keyText;
    private TextView qrPlaceholder;
    private CheckBox showNotificationCheckbox;
    private CheckBox noSleepCheckbox;
    private TextView terminalText;

    /* Persistent "No sleep" wake lock; static so it survives activity recreation */
    private static PowerManager.WakeLock noSleepWakeLock;

    /* Log window shared memory */
    private ByteBuffer logWindowBuffer;
    private Thread pollingThread;
    private volatile boolean pollingEnabled;
    private long lastEpoch = -1;
    private int logRows = 25;
    private int logCols;

    private static final int LW_OFFSET_EPOCH = 0;
    private static final int LW_OFFSET_CURRENT = 8;
    private static final int LW_OFFSET_ROWS = 12;
    private static final int LW_OFFSET_COLUMNS = 16;
    private static final int LW_OFFSET_DATA = 20;

    private final RemoteFSService.PollingListener pollingListener =
        new RemoteFSService.PollingListener() {
            @Override
            public void onEnablePolling(boolean enable) {
                if (enable) {
                    startPolling();
                } else {
                    stopPolling();
                }
            }
        };

    private static final int REQUEST_STORAGE = 100;
    private static final int REQUEST_NOTIFICATIONS = 101;

    private SettingsStore settings;
    private boolean serviceBound = false;
    private RemoteFSService boundService;

    private final Handler handler = new Handler(Looper.getMainLooper());
    private Runnable hideQrRunnable;
    private String cachedKey;
    private boolean qrVisible = false;

    private final ServiceConnection serviceConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            RemoteFSService.LocalBinder binder = (RemoteFSService.LocalBinder) service;
            boundService = binder.getService();
            serviceBound = true;
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            boundService = null;
            serviceBound = false;
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        /* Direct user to grant "manage all files" permission if missing (API 30+) */
        if (Build.VERSION.SDK_INT >= 30) {
            if (!Environment.isExternalStorageManager()) {
                Intent intent = new Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION);
                startActivity(intent);
                Toast.makeText(this,
                    "Please grant 'Allow management of all files' then return here",
                    Toast.LENGTH_LONG).show();
            }
        }

        /* Request READ/WRITE_EXTERNAL_STORAGE at runtime (API 23-29) */
        if (Build.VERSION.SDK_INT >= 23 && Build.VERSION.SDK_INT < 30) {
            if (checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED
                || checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
                requestPermissions(
                    new String[] {
                        Manifest.permission.READ_EXTERNAL_STORAGE,
                        Manifest.permission.WRITE_EXTERNAL_STORAGE
                    },
                    REQUEST_STORAGE
                );
            }
        }

        /* Request POST_NOTIFICATIONS at runtime (API 33+) */
        if (Build.VERSION.SDK_INT >= 33) {
            if (checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS)
                    != PackageManager.PERMISSION_GRANTED) {
                requestPermissions(
                    new String[] { Manifest.permission.POST_NOTIFICATIONS },
                    REQUEST_NOTIFICATIONS
                );
            }
        }

        settings = new SettingsStore(this);

        /* Detect upgrade: compare build time to last known */
        String currentBuildTime = BuildConfig.BUILD_TIME;
        if (!currentBuildTime.equals(settings.getLastBuildTime())) {
            settings.setLastBuildTime(currentBuildTime);
            if (settings.getWasRunning()) {
                restoreServer();
            }
        }

        /* Find views */
        ipRangeEdit = (EditText) findViewById(R.id.ip_range_edit);
        startButton = (Button) findViewById(R.id.start_button);
        stopButton = (Button) findViewById(R.id.stop_button);
        statusText = (TextView) findViewById(R.id.status_text);
        portInfo = (TextView) findViewById(R.id.port_info);
        qrLabel = (TextView) findViewById(R.id.qr_label);
        qrCode = (ImageView) findViewById(R.id.qr_code);
        keyText = (TextView) findViewById(R.id.key_text);
        qrPlaceholder = (TextView) findViewById(R.id.qr_placeholder);
        showNotificationCheckbox = (CheckBox) findViewById(R.id.show_notification_checkbox);
        noSleepCheckbox = (CheckBox) findViewById(R.id.no_sleep_checkbox);

        showNotificationCheckbox.setChecked(settings.getShowNotification());
        showNotificationCheckbox.setOnCheckedChangeListener(
            new CompoundButton.OnCheckedChangeListener() {
                @Override
                public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                    settings.setShowNotification(isChecked);
                    if (boundService != null) {
                        boundService.updateForegroundNotification();
                    } else if (settings.isServerRunning()) {
                        Intent intent = new Intent(MainActivity.this, RemoteFSService.class);
                        intent.putExtra("action", "update_notification");
                        startService(intent);
                    }
                }
            });

        noSleepCheckbox.setChecked(settings.getNoSleep());
        noSleepCheckbox.setOnCheckedChangeListener(
            new CompoundButton.OnCheckedChangeListener() {
                @Override
                public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                    settings.setNoSleep(isChecked);
                    applyNoSleep(isChecked);
                }
            });
        applyNoSleep(noSleepCheckbox.isChecked());

        /* Set monospace font size so 11 chars ≈ 1/3 screen width */
        float screenW = getResources().getDisplayMetrics().widthPixels
                / getResources().getDisplayMetrics().density;
        float keySize = Math.min(screenW / 20f, 16f);
        keyText.setTextSize(android.util.TypedValue.COMPLEX_UNIT_SP, keySize);

        /* QR code: 1.5 inches (240dp), capped at 9/10 screen width */
        float qrDp = Math.min(240f, screenW * 0.9f);
        int qrPx = (int) (qrDp * getResources().getDisplayMetrics().density);
        qrCode.getLayoutParams().width = qrPx;
        qrCode.getLayoutParams().height = qrPx;
        qrPlaceholder.getLayoutParams().width = qrPx;
        qrPlaceholder.getLayoutParams().height = qrPx;

        /* Terminal log window at bottom */
        terminalText = (TextView) findViewById(R.id.terminal_text);
        terminalText.setTypeface(Typeface.MONOSPACE);
        terminalText.setTextSize(android.util.TypedValue.COMPLEX_UNIT_SP, 8);
        terminalText.setTextColor(0xFF00FF00);
        terminalText.setBackgroundColor(0xFF000000);
        terminalText.setHorizontallyScrolling(false);

        /* Measure 8sp monospace character width and line height */
        Paint charPaint = new Paint();
        charPaint.setTypeface(Typeface.MONOSPACE);
        charPaint.setTextSize(8 * getResources().getDisplayMetrics().scaledDensity);
        float charWidth = charPaint.measureText("W");
        float lineHeight = charPaint.getFontSpacing();
        float density = getResources().getDisplayMetrics().density;
        logCols = (int) ((screenW * density) / charWidth);

        /* Set terminal height = rows * line height + top/bottom padding */
        int termHeight = (int) Math.ceil(logRows * lineHeight + 12.0f * density);
        terminalText.getLayoutParams().height = termHeight;

        /* Allocate direct buffer: 20 bytes header + rows * cols data */
        int bufSize = LW_OFFSET_DATA + logRows * logCols;
        logWindowBuffer = ByteBuffer.allocateDirect(bufSize);
        logWindowBuffer.order(ByteOrder.nativeOrder());
        logWindowBuffer.putInt(LW_OFFSET_ROWS, logRows);
        logWindowBuffer.putInt(LW_OFFSET_COLUMNS, logCols);

        RemoteFSService.initLogWindow(logWindowBuffer);
        RemoteFSService.setPollingListener(pollingListener);

        qrPlaceholder.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                showQrWithTimer();
            }
        });

        qrCode.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                hideQrCode();
            }
        });

        hideQrRunnable = new Runnable() {
            @Override
            public void run() {
                hideQrCode();
            }
        };

        /* Load saved settings */
        ipRangeEdit.setText(settings.getIpRange());

        /* Start button */
        startButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                onStartClicked();
            }
        });

        /* Stop button */
        stopButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                onStopClicked();
            }
        });

        updateUI();
    }

    @Override
    protected void onResume() {
        super.onResume();
        updateUI();
        /* Rebinds if server is running, e.g. after activity recreation */
        if (settings.isServerRunning() && !serviceBound) {
            bindService(new Intent(this, RemoteFSService.class),
                        serviceConnection, Context.BIND_AUTO_CREATE);
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        handler.removeCallbacks(hideQrRunnable);
        stopPolling();
        RemoteFSService.setPollingListener(null);
        RemoteFSService.clearLogWindow();
        settings.setWasRunning(settings.isServerRunning());
    }

    @Override
    public void onRequestPermissionsResult(int code, String[] perms, int[] results) {
        super.onRequestPermissionsResult(code, perms, results);
        if (code == REQUEST_STORAGE) {
            boolean granted = results.length > 0;
            for (int r : results) {
                if (r != PackageManager.PERMISSION_GRANTED) granted = false;
            }
            if (!granted) {
                Toast.makeText(this,
                    "Storage permissions denied. File access may be limited.",
                    Toast.LENGTH_LONG).show();
            }
        } else if (code == REQUEST_NOTIFICATIONS) {
            if (results.length == 0 || results[0] != PackageManager.PERMISSION_GRANTED) {
                Toast.makeText(this,
                    "Notification permission denied. Server status will not be shown.",
                    Toast.LENGTH_LONG).show();
            }
        }
    }

    /** Auto-restart server after upgrade when it was running before */
    private void restoreServer() {
        String ipRange = settings.getIpRange();
        String listenAddr = settings.getListenAddress();

        settings.setServerRunning(true);

        String keyfilePath = settings.getKeyfilePath();
        if (keyfilePath == null || keyfilePath.isEmpty()) {
            keyfilePath = getFilesDir().getAbsolutePath() + "/aeskeyfile";
        }
        File keyfile = new File(keyfilePath);
        if (!keyfile.exists()) {
            RemoteFSService.createAESKey(keyfilePath);
        }

        Intent intent = new Intent(this, RemoteFSService.class);
        intent.putExtra("action", "start");
        intent.putExtra("listen_addr", listenAddr);
        intent.putExtra("ip_range", ipRange);
        intent.putExtra("keyfile_path", settings.getKeyfilePath());

        if (settings.getShowNotification()
                && android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O) {
            startForegroundService(intent);
        } else {
            startService(intent);
        }

        /* Reinitialize native log window pointer (cleared by nativeStop) */
        RemoteFSService.initLogWindow(logWindowBuffer);

        bindService(new Intent(this, RemoteFSService.class),
                    serviceConnection, Context.BIND_AUTO_CREATE);
    }

    private void onStartClicked() {
        String ipRange = ipRangeEdit.getText().toString().trim();
        String listenAddr = "0.0.0.0";

        if (ipRange.isEmpty()) {
            Toast.makeText(this, "Please enter an IP range", Toast.LENGTH_SHORT).show();
            return;
        }

        /* Save settings */
        settings.setIpRange(ipRange);
        settings.setListenAddress(listenAddr);
        settings.setServerRunning(true);
        settings.setWasRunning(true);

        /* Create keyfile on Java side so it's ready before service starts */
        String keyfilePath = settings.getKeyfilePath();
        if (keyfilePath == null || keyfilePath.isEmpty()) {
            keyfilePath = getFilesDir().getAbsolutePath() + "/aeskeyfile";
        }
        File keyfile = new File(keyfilePath);
        if (!keyfile.exists()) {
            RemoteFSService.createAESKey(keyfilePath);
        }

        /* Start the foreground service */
        Intent intent = new Intent(this, RemoteFSService.class);
        intent.putExtra("action", "start");
        intent.putExtra("listen_addr", listenAddr);
        intent.putExtra("ip_range", ipRange);
        intent.putExtra("keyfile_path", settings.getKeyfilePath());

        if (settings.getShowNotification()
                && android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O) {
            startForegroundService(intent);
        } else {
            startService(intent);
        }

        /* Reinitialize native log window pointer (cleared by nativeStop) */
        RemoteFSService.initLogWindow(logWindowBuffer);

        /* Bind to service for status updates */
        bindService(new Intent(this, RemoteFSService.class),
                    serviceConnection, Context.BIND_AUTO_CREATE);

        Toast.makeText(this, "Server starting on " + listenAddr + ":30095", Toast.LENGTH_SHORT).show();
        updatePortInfo();
        updateUI();
    }

    private void onStopClicked() {
        Intent intent = new Intent(this, RemoteFSService.class);
        intent.putExtra("action", "stop");
        startService(intent);

        if (serviceBound) {
            unbindService(serviceConnection);
            serviceBound = false;
        }

        settings.setServerRunning(false);
        settings.setWasRunning(false);
        Toast.makeText(this, "Server stopped", Toast.LENGTH_SHORT).show();
        updatePortInfo();
        updateUI();
    }

    /** Read the WiFi-negotiated IP (or cellular fallback) and refresh the port label */
    private void updatePortInfo() {
        if (portInfo == null) return;
        String label = getString(R.string.port_info);
        String ip = getWifiIp();
        if (ip == null) {
            ip = getCellularIp();
        }
        if (ip != null) {
            label = label + " on " + ip;
        }
        portInfo.setText(label);
    }

    private String getWifiIp() {
        try {
            WifiManager wm = (WifiManager) getApplicationContext()
                    .getSystemService(Context.WIFI_SERVICE);
            if (wm == null) return null;
            WifiInfo info = wm.getConnectionInfo();
            if (info == null) return null;
            int ip = info.getIpAddress();
            if (ip == 0) return null;
            return String.format(Locale.US, "%d.%d.%d.%d",
                    ip & 0xff, (ip >> 8) & 0xff,
                    (ip >> 16) & 0xff, (ip >> 24) & 0xff);
        } catch (Exception e) {
            return null;
        }
    }

    private String getCellularIp() {
        try {
            ConnectivityManager cm = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
            if (cm == null) return null;
            Network network = cm.getActiveNetwork();
            if (network == null) return null;
            LinkProperties lp = cm.getLinkProperties(network);
            if (lp == null) return null;
            for (LinkAddress addr : lp.getLinkAddresses()) {
                InetAddress inet = addr.getAddress();
                if (inet instanceof Inet4Address && !inet.isLoopbackAddress()) {
                    return inet.getHostAddress();
                }
            }
        } catch (Exception e) {
            return null;
        }
        return null;
    }

    /** Toggle "No sleep": keep the screen on and the CPU awake */
    private void applyNoSleep(boolean enable) {
        if (enable) {
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
            if (noSleepWakeLock == null) {
                PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
                if (pm != null) {
                    noSleepWakeLock = pm.newWakeLock(
                        PowerManager.PARTIAL_WAKE_LOCK, "RemoteFS::NoSleep");
                    noSleepWakeLock.setReferenceCounted(false);
                }
            }
            if (noSleepWakeLock != null && !noSleepWakeLock.isHeld()) {
                noSleepWakeLock.acquire();
            }
        } else {
            getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
            if (noSleepWakeLock != null && noSleepWakeLock.isHeld()) {
                noSleepWakeLock.release();
            }
        }
    }

    private void updateUI() {
        boolean running = settings.isServerRunning();

        if (running) {
            statusText.setText(R.string.status_running);
            statusText.setTextColor(0xFF00AA00);
            startButton.setEnabled(false);
            stopButton.setEnabled(true);
            ipRangeEdit.setEnabled(false);

            /* Cache key so placeholder tap can re-display QR */
            cachedKey = readKeyfile();
            if (!qrVisible && cachedKey != null && cachedKey.length() == 44) {
                qrPlaceholder.setVisibility(View.VISIBLE);
            }
        } else {
            statusText.setText(R.string.status_stopped);
            statusText.setTextColor(0xFFAA0000);
            startButton.setEnabled(true);
            stopButton.setEnabled(false);
            ipRangeEdit.setEnabled(true);

            handler.removeCallbacks(hideQrRunnable);
            cachedKey = null;
            qrVisible = false;
            qrLabel.setVisibility(View.GONE);
            qrCode.setVisibility(View.GONE);
            keyText.setVisibility(View.GONE);
            qrPlaceholder.setVisibility(View.GONE);
        }
    }

    private void showQrWithTimer() {
        handler.removeCallbacks(hideQrRunnable);

        if (cachedKey != null && cachedKey.length() == 44) {
            Bitmap bmp = generateQRCode(cachedKey);
            if (bmp != null) {
                qrCode.setImageBitmap(bmp);
            }
            String keyLines = cachedKey.substring(0, 11) + "\n"
                    + cachedKey.substring(11, 22) + "\n"
                    + cachedKey.substring(22, 33) + "\n"
                    + cachedKey.substring(33, 44);
            keyText.setText(keyLines);
        }

        qrLabel.setVisibility(View.VISIBLE);
        qrCode.setVisibility(View.VISIBLE);
        keyText.setVisibility(View.VISIBLE);
        qrPlaceholder.setVisibility(View.GONE);
        qrVisible = true;

        handler.postDelayed(hideQrRunnable, 10000);
    }

    private void hideQrCode() {
        qrLabel.setVisibility(View.GONE);
        qrCode.setVisibility(View.GONE);
        keyText.setVisibility(View.GONE);
        qrPlaceholder.setVisibility(View.VISIBLE);
        qrVisible = false;
    }

    private void startPolling() {
        if (pollingThread != null) return;
        pollingEnabled = true;
        lastEpoch = -1;
        pollingThread = new Thread(new Runnable() {
            @Override
            public void run() {
                while (pollingEnabled) {
                    try {
                        Thread.sleep(500);
                    } catch (InterruptedException e) {
                        break;
                    }
                    if (!pollingEnabled) break;
                    if (logWindowBuffer == null) continue;
                    long epoch = logWindowBuffer.getLong(LW_OFFSET_EPOCH);
                    if (epoch != lastEpoch) {
                        lastEpoch = epoch;
                        handler.post(new Runnable() {
                            @Override
                            public void run() {
                                updateTerminal();
                            }
                        });
                    }
                }
            }
        });
        pollingThread.setName("LogWindow-Poll");
        pollingThread.setDaemon(true);
        pollingThread.start();
    }

    private void stopPolling() {
        pollingEnabled = false;
        if (pollingThread != null) {
            pollingThread.interrupt();
            pollingThread = null;
        }
    }

    private void updateTerminal() {
        if (logWindowBuffer == null || terminalText == null) return;
        int rows = logWindowBuffer.getInt(LW_OFFSET_ROWS);
        int cols = logWindowBuffer.getInt(LW_OFFSET_COLUMNS);
        if (rows <= 0 || cols <= 0) return;

        StringBuilder sb = new StringBuilder(rows * (cols + 1));
        for (int r = 0; r < rows; r++) {
            int base = LW_OFFSET_DATA + r * cols;
            for (int c = 0; c < cols; c++) {
                byte b = logWindowBuffer.get(base + c);
                char ch = (char) (b & 0xFF);
                if (ch < ' ') ch = ' ';
                sb.append(ch);
            }
            if (r < rows - 1) sb.append('\n');
        }
        terminalText.setText(sb.toString());
    }

    private String readKeyfile() {
        String keyfilePath = settings.getKeyfilePath();
        if (keyfilePath == null || keyfilePath.isEmpty()) {
            keyfilePath = getFilesDir().getAbsolutePath() + "/aeskeyfile";
        }
        try {
            File file = new File(keyfilePath);
            if (!file.exists()) return null;
            BufferedReader reader = new BufferedReader(new FileReader(file));
            String line = reader.readLine();
            reader.close();
            if (line != null) {
                line = line.trim();
                if (line.length() > 0) return line;
            }
        } catch (IOException e) {
            /* keyfile not ready yet */
        }
        return null;
    }

    private Bitmap generateQRCode(String data) {
        try {
            Map<EncodeHintType, Object> hints = new HashMap<>();
            hints.put(EncodeHintType.ERROR_CORRECTION, ErrorCorrectionLevel.M);
            hints.put(EncodeHintType.CHARACTER_SET, "UTF-8");

            /* ImageView layout params are already in pixels */
            int size = qrCode.getLayoutParams().width;
            if (size <= 0) size = 200;

            BitMatrix matrix = new QRCodeWriter().encode(
                data, BarcodeFormat.QR_CODE, size, size, hints);
            int[] pixels = new int[size * size];
            for (int y = 0; y < size; y++) {
                for (int x = 0; x < size; x++) {
                    pixels[y * size + x] = matrix.get(x, y) ? 0xFF000000 : 0xFFFFFFFF;
                }
            }
            Bitmap bitmap = Bitmap.createBitmap(size, size, Bitmap.Config.ARGB_8888);
            bitmap.setPixels(pixels, 0, size, 0, 0, size, size);
            return bitmap;
        } catch (Exception e) {
            Log.e("RemoteFS", "Failed to generate QR code", e);
            return null;
        }
    }
}
