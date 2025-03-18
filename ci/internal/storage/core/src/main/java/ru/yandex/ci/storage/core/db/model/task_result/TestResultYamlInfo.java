package ru.yandex.ci.storage.core.db.model.task_result;

import lombok.Value;

@Value(staticConstructor = "of")
public class TestResultYamlInfo {

    StrongModeStatus status;

    String aYamlPath;

    String service;

    boolean isOwner;

    public static TestResultYamlInfo disabled(String service) {
        return new TestResultYamlInfo(StrongModeStatus.OFF, "", service, false);
    }

    public enum StrongModeStatus {
        ON,
        OFF,
    }

}
