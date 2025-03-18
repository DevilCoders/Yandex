package ru.yandex.ci.storage.core.exceptions;

import ru.yandex.ci.core.exceptions.CiDuplicateException;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;

public class TaskAlreadyRegisteredException extends CiDuplicateException {
    public TaskAlreadyRegisteredException(CheckTaskEntity.Id id) {
        super("Task already registered: %s".formatted(id));
    }
}
