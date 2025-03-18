package ru.yandex.ci.storage.core.micrometr;

import ru.yandex.ci.metrics.CommonMetricNames;

public class StorageMetrics {
    public static final String PREFIX = "storage_";

    // Error must have exactly one tag - ERROR_TYPE
    public static final String ERRORS = PREFIX + CommonMetricNames.ERRORS;
    public static final String ERROR_TYPE = PREFIX + CommonMetricNames.ERROR_TYPE;
    public static final String ERROR_LEVEL = PREFIX + CommonMetricNames.ERROR_LEVEL;

    // Common errors
    public static final String ERROR_PARSE = "parse_error";
    public static final String ERROR_MISSING = "missing";
    public static final String ERROR_FINISHED_STATE = "finished_state";
    public static final String ERROR_VALIDATION = "validation";

    public static final String RESULTS_PROCESSED = PREFIX + "results_processed";

    public static final String MESSAGE_CASE = PREFIX + "message_case";

    public static final String QUEUE = PREFIX + "queue";


    private StorageMetrics() {

    }
}
