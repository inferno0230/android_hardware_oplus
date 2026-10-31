package com.oplus.widget;

import android.content.Context;
import android.graphics.drawable.Drawable;

/* loaded from: classes.dex */
public class OplusItem {
    public static final int ITEM_FIFTH = 4;
    public static final int ITEM_FIRST = 0;
    public static final int ITEM_FOURTH = 3;
    public static final int ITEM_SECOND = 1;
    public static final int ITEM_THIRD = 2;
    private Drawable mBackgroud;
    private Context mContext;
    private Drawable mIcon;
    private String mLabel;
    private OnItemClickListener mOnItemClickListener;
    private String mPackageName;
    private boolean mPinned = false;
    private String mText;

    /* loaded from: classes.dex */
    public interface OnItemClickListener {
        void OnMenuItemClick(int i);
    }

    /* loaded from: classes.dex */
    public static class Builder {
        private OplusItem ci = new OplusItem();

        public Builder(Context context) {
            this.ci.mContext = context;
        }

        public Builder setText(String text) {
            this.ci.mText = text;
            return this;
        }

        public Builder setText(int textResId) {
            this.ci.mText = this.ci.getContext().getString(textResId);
            return this;
        }

        public Builder setLabel(int labelId) {
            this.ci.mLabel = this.ci.getContext().getString(labelId);
            return this;
        }

        public Builder setLabel(String label) {
            this.ci.mLabel = label;
            return this;
        }

        public Builder setIcon(Drawable icon) {
            this.ci.mIcon = icon;
            return this;
        }

        public Builder setIcon(int iconResId) {
            this.ci.mIcon = this.ci.getContext().getResources().getDrawable(iconResId);
            return this;
        }

        public Builder setBackgroud(Drawable background) {
            this.ci.mBackgroud = background;
            return this;
        }

        public Builder setBackgroud(int bgResId) {
            this.ci.mBackgroud = this.ci.getContext().getResources().getDrawable(bgResId);
            return this;
        }

        public Builder setOnItemClickListener(OnItemClickListener e) {
            this.ci.mOnItemClickListener = e;
            return this;
        }

        private Builder setPinned(boolean pinned) {
            this.ci.mPinned = pinned;
            return this;
        }

        public Builder setPackageName(String packageName) {
            this.ci.mPackageName = packageName;
            return this;
        }

        public OplusItem create() {
            return this.ci;
        }
    }

    public String getText() {
        return this.mText;
    }

    public void setText(String mText) {
        this.mText = mText;
    }

    public String getLabel() {
        return this.mLabel;
    }

    public void setLabel(String subLabel) {
        this.mLabel = subLabel;
    }

    public Drawable getIcon() {
        return this.mIcon;
    }

    public void setIcon(Drawable mIcon) {
        this.mIcon = mIcon;
    }

    public Drawable getBackgroud() {
        return this.mBackgroud;
    }

    public void setBackgroud(Drawable backgroud) {
        this.mBackgroud = backgroud;
    }

    public Context getContext() {
        return this.mContext;
    }

    public void setContext(Context context) {
        this.mContext = context;
    }

    public OnItemClickListener getOnItemClickListener() {
        return this.mOnItemClickListener;
    }

    public void setOnItemClickListener(OnItemClickListener onItemClickListener) {
        this.mOnItemClickListener = onItemClickListener;
    }

    public boolean isPinned() {
        return this.mPinned;
    }

    public void setPinned(Boolean pinned) {
        this.mPinned = pinned.booleanValue();
    }

    public String getPackageName() {
        return this.mPackageName;
    }

    public void setPackageName(String packageName) {
        this.mPackageName = packageName;
    }
}
