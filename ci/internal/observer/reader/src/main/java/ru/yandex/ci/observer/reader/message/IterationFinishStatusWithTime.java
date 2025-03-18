package ru.yandex.ci.observer.reader.message;

import java.time.Instant;

import lombok.Value;

import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationEntity;
import ru.yandex.ci.storage.core.Common;

@Value
public class IterationFinishStatusWithTime {
    CheckIterationEntity.Id iterationId;
    Common.CheckStatus status;
    Instant time;
}
