package com.oplus.wrapper.util;

public class StatsEvent {

    private final android.util.StatsEvent mStatsEvent;

    private StatsEvent(android.util.StatsEvent statsEvent) {
        mStatsEvent = statsEvent;
    }

    android.util.StatsEvent getStatsEvent() {
        return mStatsEvent;
    }

    public static Builder newBuilder() {
        return new Builder(android.util.StatsEvent.newBuilder());
    }

    public static final class Builder {

        private final android.util.StatsEvent.Builder mBuilder;

        private Builder(android.util.StatsEvent.Builder builder) {
            mBuilder = builder;
        }

        public Builder setAtomId(int atomId) {
            mBuilder.setAtomId(atomId);
            return this;
        }

        public Builder writeBoolean(boolean value) {
            mBuilder.writeBoolean(value);
            return this;
        }

        public Builder writeInt(int value) {
            mBuilder.writeInt(value);
            return this;
        }

        public Builder writeLong(long value) {
            mBuilder.writeLong(value);
            return this;
        }

        public Builder writeString(String value) {
            mBuilder.writeString(value);
            return this;
        }

        public Builder usePooledBuffer() {
            mBuilder.usePooledBuffer();
            return this;
        }

        public StatsEvent build() {
            return new StatsEvent(mBuilder.build());
        }
    }
}
