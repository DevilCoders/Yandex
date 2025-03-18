package ru.yandex.ci.storage.core.yt;

import io.temporal.activity.ActivityInterface;
import io.temporal.activity.ActivityMethod;

import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;

@ActivityInterface
public interface YtExportActivity {

    @ActivityMethod
    void exportIteration(CheckIterationEntity.Id id);
}
