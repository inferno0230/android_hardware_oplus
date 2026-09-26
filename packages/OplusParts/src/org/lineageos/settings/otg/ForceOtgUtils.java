/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package org.lineageos.settings.otg;

import android.content.Context;
import android.content.SharedPreferences;
import android.util.Log;

import androidx.preference.PreferenceManager;

import org.lineageos.settings.utils.FileUtils;

public final class ForceOtgUtils {
    private static final String TAG = "ForceOtgUtils";

    private static final String KEY_FORCE_OTG_ENABLED = "force_otg_enabled";

    private static final String FORCE_OTG_NODE = "/sys/class/oplus_chg/usb/otg_switch";

    private ForceOtgUtils() {
        // Utility class
    }

    public static boolean isSupported() {
        return FileUtils.fileExists(FORCE_OTG_NODE);
    }

    public static boolean isForceOtgEnabled(Context context) {
        if (!isSupported()) {
            return false;
        }

        final String nodeValue = FileUtils.readOneLine(FORCE_OTG_NODE);
        if (nodeValue != null) {
            final String trimmed = nodeValue.trim();
            if ("0".equals(trimmed) || "1".equals(trimmed)) {
                return "1".equals(trimmed);
            }
        }

        return getSharedPreferences(context)
                .getBoolean(KEY_FORCE_OTG_ENABLED, false);
    }

    public static boolean setForceOtgEnabled(Context context, boolean enabled) {
        if (!isSupported()) {
            return false;
        }

        if (!writeForceOtgNode(enabled)) {
            Log.w(TAG, "Failed to write force OTG node: " + FORCE_OTG_NODE);
            return false;
        }

        getSharedPreferences(context)
                .edit()
                .putBoolean(KEY_FORCE_OTG_ENABLED, enabled)
                .apply();

        return true;
    }

    public static void restoreForceOtg(Context context) {
        if (!isSupported()) {
            return;
        }

        final boolean enabled = getSharedPreferences(context)
                .getBoolean(KEY_FORCE_OTG_ENABLED, false);

        if (!writeForceOtgNode(enabled)) {
            Log.w(TAG, "Failed to restore force OTG state to node: " + FORCE_OTG_NODE);
        }
    }

    private static boolean writeForceOtgNode(boolean enabled) {
        return FileUtils.writeLine(FORCE_OTG_NODE, enabled ? "1" : "0");
    }

    private static SharedPreferences getSharedPreferences(Context context) {
        return PreferenceManager.getDefaultSharedPreferences(context);
    }
}
