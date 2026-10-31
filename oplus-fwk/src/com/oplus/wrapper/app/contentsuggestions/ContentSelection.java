package com.oplus.wrapper.app.contentsuggestions;

import android.os.Bundle;

public final class ContentSelection {
    private final android.app.contentsuggestions.ContentSelection mContentSelection;

    public ContentSelection(String selectionId, Bundle extras) {
        mContentSelection = new android.app.contentsuggestions.ContentSelection(selectionId, extras);
    }

    ContentSelection(android.app.contentsuggestions.ContentSelection contentSelection) {
        mContentSelection = contentSelection;
    }

    public String getId() {
        return this.mContentSelection.getId();
    }

    public Bundle getExtras() {
        return this.mContentSelection.getExtras();
    }

    android.app.contentsuggestions.ContentSelection getContentSelection() {
        return this.mContentSelection;
    }
}
