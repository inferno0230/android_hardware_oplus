package com.oplus.app;

import java.util.Map;

import java.util.HashMap;

import android.os.UserHandle;

public class OPlusAccessControlManager {
    
    private static OPlusAccessControlManager sOPlusAccessControlManager = null;

    public static final int USER_CURRENT = UserHandle.myUserId();

    public static OPlusAccessControlManager getInstance() {
        if (sOPlusAccessControlManager == null) {
            sOPlusAccessControlManager = new OPlusAccessControlManager();
        }
        return sOPlusAccessControlManager;
    }

    public boolean isEncryptPass(String packageName, int userId) {
        return true;
    }

    public boolean isEncryptedPackage(String packageName, int userId) {
        return true;
    }

    public boolean getAccessControlEnabled(String packageName, int userId) {
        return false;
    }

    public boolean getApplicationAccessControlEnabledAsUser(String packageName, int userId) {
        return false;
    }

    public Map<String, Integer> getAccessControlAppsInfo(String type, int userId) {
        return new HashMap<>();
    }

    public Map<String, Integer> getPrivacyAppInfo(int userId) {
        return new HashMap<>();
    }
}
