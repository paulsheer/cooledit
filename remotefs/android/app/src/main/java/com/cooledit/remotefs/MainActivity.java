package com.cooledit.remotefs;

import android.Manifest;
import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.IBinder;
import android.provider.Settings;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import com.google.zxing.BarcodeFormat;
import com.google.zxing.EncodeHintType;
import com.google.zxing.qrcode.QRCodeWriter;
import com.google.zxing.qrcode.decoder.ErrorCorrectionLevel;
import com.google.zxing.common.BitMatrix;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.HashMap;
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
    private TextView qrLabel;
    private ImageView qrCode;
    private TextView keyText;

    private static final int REQUEST_STORAGE = 100;
    private static final int REQUEST_NOTIFICATIONS = 101;

    private SettingsStore settings;
    private boolean serviceBound = false;
    private RemoteFSService boundService;

    private final ServiceConnection serviceConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            serviceBound = true;
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
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

        /* Find views */
        ipRangeEdit = (EditText) findViewById(R.id.ip_range_edit);
        startButton = (Button) findViewById(R.id.start_button);
        stopButton = (Button) findViewById(R.id.stop_button);
        statusText = (TextView) findViewById(R.id.status_text);
        qrLabel = (TextView) findViewById(R.id.qr_label);
        qrCode = (ImageView) findViewById(R.id.qr_code);
        keyText = (TextView) findViewById(R.id.key_text);

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

        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O) {
            startForegroundService(intent);
        } else {
            startService(intent);
        }

        /* Bind to service for status updates */
        bindService(new Intent(this, RemoteFSService.class),
                    serviceConnection, Context.BIND_AUTO_CREATE);

        Toast.makeText(this, "Server starting on " + listenAddr + ":50095", Toast.LENGTH_SHORT).show();
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
        Toast.makeText(this, "Server stopped", Toast.LENGTH_SHORT).show();
        updateUI();
    }

    private void updateUI() {
        boolean running = settings.isServerRunning();

        if (running) {
            statusText.setText(R.string.status_running);
            statusText.setTextColor(0xFF00AA00);
            startButton.setEnabled(false);
            stopButton.setEnabled(true);
            ipRangeEdit.setEnabled(false);

            /* Load AES key and show QR code */
            String key = readKeyfile();
            if (key != null && key.length() == 44) {
                Bitmap bmp = generateQRCode(key);
                if (bmp != null) {
                    qrCode.setImageBitmap(bmp);
                    qrLabel.setVisibility(View.VISIBLE);
                    qrCode.setVisibility(View.VISIBLE);
                }
                String keyLines = key.substring(0, 11) + "\n"
                        + key.substring(11, 22) + "\n"
                        + key.substring(22, 33) + "\n"
                        + key.substring(33, 44);
                keyText.setText(keyLines);
                keyText.setVisibility(View.VISIBLE);
            }
        } else {
            statusText.setText(R.string.status_stopped);
            statusText.setTextColor(0xFFAA0000);
            startButton.setEnabled(true);
            stopButton.setEnabled(false);
            ipRangeEdit.setEnabled(true);
            qrLabel.setVisibility(View.GONE);
            qrCode.setVisibility(View.GONE);
            keyText.setVisibility(View.GONE);
        }
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

            /* Get target pixel size from the ImageView layout params */
            int size = qrCode.getLayoutParams().width;
            if (size <= 0) size = 200;
            size = (int) (size * getResources().getDisplayMetrics().density);

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
            return null;
        }
    }
}
