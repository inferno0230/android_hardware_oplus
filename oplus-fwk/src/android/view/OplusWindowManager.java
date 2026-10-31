package android.view;

import java.util.List;

import java.util.ArrayList;

import android.graphics.Rect;

public class OplusWindowManager {

    public OplusWindowManager() {}

    public void requestKeyguard(String command) {}

    public boolean setPreferredDisplayMode(int modeId) {
        return false;
    }

    public boolean isInputShow() {
        return false;
    }

    public void getFocusedWindowFrame(Rect frame) {}

    public int getLongshotSurfaceLayerByType(int type) {
        return 0;
    }

    public List<Object> getAllVisibleWindowInfo() {
        return new ArrayList<>();
    }

    public List<Rect> getSplitAreaRegion() {
        return new ArrayList<>();
    }
}
