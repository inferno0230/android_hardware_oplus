package com.oplus.wrapper.media;

public class AudioManager {

    public static final int STREAM_SYSTEM_ENFORCED = android.media.AudioManager.STREAM_SYSTEM_ENFORCED;

    private final android.media.AudioManager mAudioManager;

    public AudioManager(android.media.AudioManager audioManager) {
        mAudioManager = audioManager;
    }

    public int getRingerModeInternal() {
        return mAudioManager.getRingerModeInternal();
    }
}
