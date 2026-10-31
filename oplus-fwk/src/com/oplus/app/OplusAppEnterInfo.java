package com.oplus.app;

import android.os.Bundle;

import android.content.Intent;

public class OplusAppEnterInfo {

    public OplusAppEnterInfo() {}

    public Intent intent;
    public String targetName;
    public String launchedFromPackage;
    public boolean firstStart;
    public boolean multiApp;
    public int windowMode;
    public Bundle extension = new Bundle();

    @Override
    public String toString() {
        return "OplusAppEnterInfo{targetName=" + targetName + "}";
    }
}
