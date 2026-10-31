package com.oplus.app;

import java.util.HashSet;

import java.util.ArrayList;

import java.util.List;

public class OplusAppSwitchConfig {

    public void addAppConfig(int type, List<String> list) {}

    public HashSet<String> mActivitySet = new HashSet<>();
    public int observerFingerPrint;

    public OplusAppSwitchConfig() {}

    public List<String> getConfigs(int type) {
        return new ArrayList<>();
    }

    public void removeAppConfig(int type) {}

    @Override
    public String toString() {
        return "OplusAppSwitchConfig{}";
    }
}
