/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package org.lineageos.settings.ltpo;

import android.content.Context;
import android.content.SharedPreferences;
import android.util.Log;

import androidx.preference.PreferenceManager;

import org.lineageos.settings.utils.FileUtils;

public final class LtpoHelper {
    private static final String TAG = "LtpoHelper";

    private static final String KEY_LTPO_ENABLED = "ltpo_enabled";
    private static final String LTPO_NODE = "/sys/kernel/oplus_display/ltpo_enable";
    private static final String TEST_TE_NODE = "/sys/kernel/oplus_display/test_te";

    private LtpoHelper() {
        // Utility class
    }

    public static boolean isSupported() {
        return FileUtils.fileExists(LTPO_NODE) && isTestTeSupported();
    }

    public static boolean isLtpoEnabled(Context context) {
        if (!isSupported()) {
            return false;
        }

        final String nodeValue = FileUtils.readOneLine(LTPO_NODE);
        if (nodeValue != null) {
            final String trimmed = nodeValue.trim();
            if ("0".equals(trimmed) || "1".equals(trimmed)) {
                return "1".equals(trimmed);
            }
        }

        return getSharedPreferences(context).getBoolean(KEY_LTPO_ENABLED, true);
    }

    public static boolean setLtpoEnabled(Context context, boolean enabled) {
        if (!isSupported()) {
            return false;
        }

        if (!writeLtpoNode(enabled)) {
            Log.w(TAG, "Failed to write LTPO node: " + LTPO_NODE);
            return false;
        }

        getSharedPreferences(context)
                .edit()
                .putBoolean(KEY_LTPO_ENABLED, enabled)
                .apply();

        return true;
    }

    public static void restoreLtpo(Context context) {
        if (!isSupported()) {
            return;
        }

        final boolean enabled = getSharedPreferences(context)
                .getBoolean(KEY_LTPO_ENABLED, true);

        if (!writeLtpoNode(enabled)) {
            Log.w(TAG, "Failed to restore LTPO state to node: " + LTPO_NODE);
        }
    }

    private static boolean writeLtpoNode(boolean enabled) {
        return FileUtils.writeLine(LTPO_NODE, enabled ? "1" : "0");
    }

    private static boolean isTestTeSupported() {
        if (!FileUtils.fileExists(TEST_TE_NODE)) {
            return false;
        }

        final String nodeValue = FileUtils.readOneLine(TEST_TE_NODE);
        if (nodeValue == null) {
            return false;
        }

        final String trimmed = nodeValue.trim();
        if (trimmed.isEmpty() || "invalid".equalsIgnoreCase(trimmed)) {
            return false;
        }

        try {
            return Integer.parseInt(trimmed) > 0;
        } catch (NumberFormatException e) {
            Log.w(TAG, "Unexpected test_te value: " + trimmed, e);
            return false;
        }
    }

    private static SharedPreferences getSharedPreferences(Context context) {
        return PreferenceManager.getDefaultSharedPreferences(context);
    }
}
