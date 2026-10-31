package com.oplus.osense.eventinfo;

import java.util.HashSet;

public class EventConfig {

    private HashSet<OsenseConfig> mOsenseConfigSet;

    public EventConfig(HashSet<OsenseConfig> osenseConfigSet) {
        mOsenseConfigSet = osenseConfigSet;
    }

    public HashSet<OsenseConfig> getOsenseConfigSet() {
        return mOsenseConfigSet;
    }

    public void setOsenseConfigSet(HashSet<OsenseConfig> osenseConfigSet) {
        mOsenseConfigSet = osenseConfigSet;
    }

    public java.util.Set<OsenseConfig> getEventSet() {
        return mOsenseConfigSet;
    }
}
