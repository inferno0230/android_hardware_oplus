package com.oplus.app;

import android.os.Bundle;

public class OplusAppExitInfo {

    public OplusAppExitInfo() {}

    public String targetName;
    public String resumingPackageName;
    public String resumingActivityName;
    public boolean hasResumingActivity;
    public boolean isResumingFirstStart;
    public boolean isResumingMultiApp;
    public int resumingWindowMode;
    public Bundle extension = new Bundle();

    @Override
    public String toString() {
        return "OplusAppExitInfo{targetName=" + targetName + "}";
    }
}
