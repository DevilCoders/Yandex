package ru.yandex.ci.storage.api.tests;

import lombok.Value;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.task_result.TestResultEntity;

@Value
public class LaunchesByStatus {
    Common.TestStatus status;
    long numberOfLaunches;
    TestResultEntity lastRun;
}
