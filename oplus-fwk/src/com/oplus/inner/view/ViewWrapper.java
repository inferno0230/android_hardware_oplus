package com.oplus.inner.view;

import android.view.View;
import java.lang.reflect.Field;

public class ViewWrapper {
    // COUI scroll containers (COUINestedScrollView/COUIScrollView) commit their scroll
    // offset through these accessors. They MUST write View.mScrollX/mScrollY directly:
    // calling the public View.setScrollX/setScrollY re-dispatches to the overridden
    // scrollTo(), which calls back here -> infinite recursion -> StackOverflowError and
    // the list never scrolls. This mirrors the real ColorOS "inner" accessor.
    public static void setScrollXForColor(View view, int scrollX) {
        setField(view, "mScrollX", scrollX);
    }

    public static void setScrollYForColor(View view, int scrollY) {
        setField(view, "mScrollY", scrollY);
    }

    private static void setField(View view, String name, int value) {
        if (view == null) return;
        try {
            Field f = View.class.getDeclaredField(name);
            f.setAccessible(true);
            f.setInt(view, value);
        } catch (ReflectiveOperationException ignored) {
        }
    }
}
