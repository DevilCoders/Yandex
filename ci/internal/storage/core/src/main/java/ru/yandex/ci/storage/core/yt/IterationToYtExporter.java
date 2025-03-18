package ru.yandex.ci.storage.core.yt;

import java.util.concurrent.ExecutionException;

import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;

public interface IterationToYtExporter {
    void export(CheckIterationEntity.Id id) throws ExecutionException, InterruptedException;
}
