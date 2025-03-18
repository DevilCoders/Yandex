package ru.yandex.ci.storage.reader.export;

import lombok.Value;

import ru.yandex.ci.storage.core.TaskMessages;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;

@Value
public class CheckTaskTestResult {
    CheckEntity check;
    CheckTaskEntity task;
    int partition;
    TaskMessages.AutocheckTestResult testResult;
}
