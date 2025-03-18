package ru.yandex.ci.storage.core.cache;

public class CacheConstants {
    public static final String METRIC_NAME = "storage-cache";
    public static final String ACTION = "action";
    public static final String NAME = "name";

    // actions

    // loading from db, higher is better.
    public static final String EMPTY_LOAD = "empty-load";   // no queries to db.
    public static final String GROUP_LOAD = "group-load";   // single query for several entities.
    public static final String SINGLE_LOAD = "single-load"; // single query for one entity.

    private CacheConstants() {

    }
}
