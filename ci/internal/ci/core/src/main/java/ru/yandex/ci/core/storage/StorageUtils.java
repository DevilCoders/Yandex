package ru.yandex.ci.core.storage;

public class StorageUtils {

    private static final String RESTART_JOB_PREFIX = "restart-";

    private StorageUtils() {

    }

    public static String toRestartJobName(String jobName) {
        return RESTART_JOB_PREFIX + jobName;
    }

    public static String removeRestartJobPrefix(String jobName) {
        return jobName.startsWith(RESTART_JOB_PREFIX)
                ? jobName.substring(RESTART_JOB_PREFIX.length())
                : jobName;
    }

}
