package com.cooledit.remotefs;

import android.content.SharedPreferences;
import android.content.Context;
import android.preference.PreferenceManager;

/**
 * Simple SharedPreferences wrapper for persisting settings.
 */
public class SettingsStore {

    private static final String KEY_IP_RANGE = "ip_range";
    private static final String KEY_LISTEN_ADDR = "listen_addr";
    private static final String KEY_KEYFILE_PATH = "keyfile_path";
    private static final String KEY_SERVER_RUNNING = "server_running";
    private static final String KEY_SHOW_NOTIFICATION = "show_notification";
    private static final String KEY_NO_SLEEP = "no_sleep";
    private static final String KEY_LAST_BUILD_TIME = "last_build_time";
    private static final String KEY_WAS_RUNNING = "was_running";

    private static final String DEFAULT_LISTEN_ADDR = "0.0.0.0";
    private static final String DEFAULT_IP_RANGE = "127.0.0.0/8";
    private static final String DEFAULT_KEYFILE = "";

    private final SharedPreferences prefs;

    public SettingsStore(Context context) {
        prefs = PreferenceManager.getDefaultSharedPreferences(context);
    }

    public String getListenAddress() {
        return prefs.getString(KEY_LISTEN_ADDR, DEFAULT_LISTEN_ADDR);
    }

    public void setListenAddress(String addr) {
        prefs.edit().putString(KEY_LISTEN_ADDR, addr).apply();
    }

    public String getIpRange() {
        return prefs.getString(KEY_IP_RANGE, DEFAULT_IP_RANGE);
    }

    public void setIpRange(String range) {
        prefs.edit().putString(KEY_IP_RANGE, range).apply();
    }

    public String getKeyfilePath() {
        return prefs.getString(KEY_KEYFILE_PATH, DEFAULT_KEYFILE);
    }

    public void setKeyfilePath(String path) {
        prefs.edit().putString(KEY_KEYFILE_PATH, path).apply();
    }

    public boolean isServerRunning() {
        return prefs.getBoolean(KEY_SERVER_RUNNING, false);
    }

    public void setServerRunning(boolean running) {
        prefs.edit().putBoolean(KEY_SERVER_RUNNING, running).commit();
    }

    public boolean getShowNotification() {
        return prefs.getBoolean(KEY_SHOW_NOTIFICATION, true);
    }

    public void setShowNotification(boolean show) {
        prefs.edit().putBoolean(KEY_SHOW_NOTIFICATION, show).commit();
    }

    public boolean getNoSleep() {
        return prefs.getBoolean(KEY_NO_SLEEP, false);
    }

    public void setNoSleep(boolean noSleep) {
        prefs.edit().putBoolean(KEY_NO_SLEEP, noSleep).commit();
    }

    public String getLastBuildTime() {
        return prefs.getString(KEY_LAST_BUILD_TIME, "");
    }

    public void setLastBuildTime(String time) {
        prefs.edit().putString(KEY_LAST_BUILD_TIME, time).apply();
    }

    public boolean getWasRunning() {
        return prefs.getBoolean(KEY_WAS_RUNNING, false);
    }

    public void setWasRunning(boolean running) {
        prefs.edit().putBoolean(KEY_WAS_RUNNING, running).commit();
    }
}
