package com.oplus.osense.eventinfo;

import android.os.Bundle;

public class OsenseEventResult {

    private int mEventType;
    private int mEventStateType;
    private Bundle mExtraData = new Bundle();

    public int getEventType() {
        return mEventType;
    }

    public int getEventStateType() {
        return mEventStateType;
    }

    public Bundle getExtraData() {
        return mExtraData;
    }
}
