package com.oplus.wrapper.content.res;

import com.oplus.wrapper.app.WindowConfiguration;

public class Configuration {

    private final android.content.res.Configuration mConfiguration;

    public Configuration(android.content.res.Configuration configuration) {
        mConfiguration = configuration;
    }

    public WindowConfiguration getWindowConfiguration() {
        return new WindowConfiguration(mConfiguration.windowConfiguration);
    }
}
