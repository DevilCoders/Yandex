package ru.yandex.ci.tms.data;

public class SolomonAlert {
    private final String projectId;
    private final String alertId;

    public SolomonAlert(String projectId, String alertId) {
        this.projectId = projectId;
        this.alertId = alertId;
    }

    public String getProjectId() {
        return projectId;
    }

    public String getAlertId() {
        return alertId;
    }

    public enum Status {
        OK,
        WARNING,
        ALARM,
        NO_DATA,
        ERROR,
        UNRECOGNIZED
    }
}
