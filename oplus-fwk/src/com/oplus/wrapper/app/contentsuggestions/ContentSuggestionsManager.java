package com.oplus.wrapper.app.contentsuggestions;

import android.content.Context;
import android.graphics.Bitmap;
import android.os.Bundle;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Executor;

public class ContentSuggestionsManager {
    public static final String EXTRA_BITMAP = getExtraBitmap();
    private final android.app.contentsuggestions.ContentSuggestionsManager mContentSuggestionsManager;

    public interface ClassificationsCallback {
        void onContentClassificationsAvailable(int i, List<ContentClassification> list);
    }

    public interface SelectionsCallback {
        void onContentSelectionsAvailable(int i, List<ContentSelection> list);
    }

    public ContentSuggestionsManager(Context context) {
        mContentSuggestionsManager = (android.app.contentsuggestions.ContentSuggestionsManager) context.getSystemService("content_suggestions");
    }

    public void provideContextImage(Bitmap bitmap, Bundle imageContextRequestExtras) {
        mContentSuggestionsManager.provideContextImage(bitmap, imageContextRequestExtras);
    }

    public void provideContextImage(int taskId, Bundle imageContextRequestExtras) {
        mContentSuggestionsManager.provideContextImage(taskId, imageContextRequestExtras);
    }

    public void suggestContentSelections(SelectionsRequest request, Executor callbackExecutor, SelectionsCallback callback) {
        SelectionsCallbackDelegate selectionsCallback = new SelectionsCallbackDelegate(callback);
        mContentSuggestionsManager.suggestContentSelections(request.getSelectionsRequest(), callbackExecutor, selectionsCallback);
    }

    public void classifyContentSelections(ClassificationsRequest request, Executor callbackExecutor, ClassificationsCallback callback) {
        ClassificationsCallbackDelegate classificationsCallback = new ClassificationsCallbackDelegate(callback);
        mContentSuggestionsManager.classifyContentSelections(request.getClassificationsRequest(), callbackExecutor, classificationsCallback);
    }

    public void notifyInteraction(String requestId, Bundle interaction) {
        mContentSuggestionsManager.notifyInteraction(requestId, interaction);
    }

    private static String getExtraBitmap() {
        return "android.contentsuggestions.extra.BITMAP";
    }

    private static class SelectionsCallbackDelegate implements android.app.contentsuggestions.ContentSuggestionsManager.SelectionsCallback {
        private final SelectionsCallback mCallback;

        public SelectionsCallbackDelegate(SelectionsCallback callback) {
            mCallback = callback;
        }

        public void onContentSelectionsAvailable(int statusCode, List<android.app.contentsuggestions.ContentSelection> selections) {
            List<ContentSelection> delegateSelections = new ArrayList<>(selections.size());
            for (android.app.contentsuggestions.ContentSelection selection : selections) {
                delegateSelections.add(new ContentSelection(selection));
            }
            mCallback.onContentSelectionsAvailable(statusCode, delegateSelections);
        }
    }

    private static class ClassificationsCallbackDelegate implements android.app.contentsuggestions.ContentSuggestionsManager.ClassificationsCallback {
        private final ClassificationsCallback mCallback;

        public ClassificationsCallbackDelegate(ClassificationsCallback callback) {
            mCallback = callback;
        }

        public void onContentClassificationsAvailable(int statusCode, List<android.app.contentsuggestions.ContentClassification> classifications) {
            List<ContentClassification> delegateClassifications = new ArrayList<>(classifications.size());
            for (android.app.contentsuggestions.ContentClassification classification : classifications) {
                delegateClassifications.add(new ContentClassification(classification));
            }
            mCallback.onContentClassificationsAvailable(statusCode, delegateClassifications);
        }
    }
}
