package com.oplus.wrapper.os;

public final class Debug {

    private Debug() {}

    public static String getCallers(int depth) {
        return android.os.Debug.getCallers(depth);
    }
}
