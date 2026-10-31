package com.oplus.wrapper.app.contentsuggestions;

import android.os.Bundle;

public final class ContentClassification {
    private final android.app.contentsuggestions.ContentClassification mContentClassification;

    ContentClassification(android.app.contentsuggestions.ContentClassification contentClassification) {
        mContentClassification = contentClassification;
    }

    public String getId() {
        return this.mContentClassification.getId();
    }

    public Bundle getExtras() {
        return this.mContentClassification.getExtras();
    }
}
