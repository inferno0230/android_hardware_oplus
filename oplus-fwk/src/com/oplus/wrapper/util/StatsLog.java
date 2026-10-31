package com.oplus.wrapper.util;

public class StatsLog {

    private StatsLog() {}

    public static void write(StatsEvent statsEvent) {
        android.util.StatsLog.write(statsEvent.getStatsEvent());
    }
}
