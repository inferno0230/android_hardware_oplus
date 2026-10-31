package com.oplus.wrapper.os.storage;

public class StorageManager {

    private final android.os.storage.StorageManager mStorageManager;

    public StorageManager(android.os.storage.StorageManager storageManager) {
        mStorageManager = storageManager;
    }

    public android.os.storage.StorageVolume[] getVolumeList() {
        return mStorageManager.getVolumeList();
    }

    public String[] getVolumePaths() {
        return mStorageManager.getVolumePaths();
    }

    public String getVolumeState(String mountPoint) {
        return mStorageManager.getVolumeState(mountPoint);
    }
}
