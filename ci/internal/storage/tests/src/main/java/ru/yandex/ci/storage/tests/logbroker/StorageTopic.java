package ru.yandex.ci.storage.tests.logbroker;

public final class StorageTopic {

    public static final String MAIN = "MAIN";
    public static final String SHARD_IN = "SHARD_IN";
    public static final String SHARD_OUT = "SHARD_OUT";
    public static final String EVENTS = "EVENTS";
    public static final String POST_PROCESSOR = "POST_PROCESSOR";

    private StorageTopic() {
    }
}
