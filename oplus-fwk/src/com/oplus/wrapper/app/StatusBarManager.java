package com.oplus.wrapper.app;

public class StatusBarManager {

    private final android.app.StatusBarManager mStatusBarManager;

    public StatusBarManager(android.app.StatusBarManager statusBarManager) {
        mStatusBarManager = statusBarManager;
    }

    public void expandNotificationsPanel() {
        mStatusBarManager.expandNotificationsPanel();
    }

    public void collapsePanels() {
        mStatusBarManager.collapsePanels();
    }
}
