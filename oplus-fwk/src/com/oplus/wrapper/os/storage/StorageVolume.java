package com.oplus.wrapper.os.storage;

public class StorageVolume {

    private final android.os.storage.StorageVolume mStorageVolume;

    public StorageVolume(android.os.storage.StorageVolume storageVolume) {
        mStorageVolume = storageVolume;
    }

    public String getPath() {
        return mStorageVolume.getPath();
    }
}
