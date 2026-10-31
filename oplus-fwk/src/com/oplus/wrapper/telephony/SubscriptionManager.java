package com.oplus.wrapper.telephony;

public class SubscriptionManager {

    public static int[] getSubId(int slotIndex) {
        return android.telephony.SubscriptionManager.getSubId(slotIndex);
    }
}
