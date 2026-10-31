package com.oplus.wrapper.content.pm;

public class PackageManager {

    private final android.content.pm.PackageManager mPackageManager;

    public PackageManager(android.content.pm.PackageManager packageManager) {
        mPackageManager = packageManager;
    }

    public String getAppPredictionServicePackageName() {
        return mPackageManager.getAppPredictionServicePackageName();
    }
}
