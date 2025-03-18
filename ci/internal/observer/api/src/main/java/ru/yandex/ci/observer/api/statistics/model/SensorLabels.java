package ru.yandex.ci.observer.api.statistics.model;

public class SensorLabels {
    public static final String PERCENTILE = "percentile";
    public static final String STAGE = "stage";
    public static final String POOL = "advised_pool";
    public static final String ITERATION = "iteration";
    public static final String WINDOW = "window";
    public static final String CHECK_TYPE = "check_type";
    public static final String CHECK_ITERATION_TYPE = "check_iteration_type";
    public static final String METRIC = "metric";
    public static final String STAGE_TYPE = "stage_type";
    public static final String CREATE_SYSTEM = "create_system";
    public static final String JOB_NAME = "job_name";

    public static final String ANY_POOL = "ANY_POOL";
    public static final String ANY_CHECK_ITERATION_TYPE = "ANY";

    private SensorLabels() {
    }
}
