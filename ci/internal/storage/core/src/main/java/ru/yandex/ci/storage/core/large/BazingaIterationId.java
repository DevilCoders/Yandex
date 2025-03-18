package ru.yandex.ci.storage.core.large;

import lombok.Value;

import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.misc.bender.annotation.BenderBindAllFields;

@Value
@BenderBindAllFields
public class BazingaIterationId {
    Long id;
    int iterationType;
    int number;

    public BazingaIterationId(CheckIterationEntity.Id id) {
        this.id = id.getCheckId().getId();
        this.iterationType = id.getIterationTypeNumber();
        this.number = id.getNumber();
    }

    public CheckIterationEntity.Id getIterationId() {
        return new CheckIterationEntity.Id(CheckEntity.Id.of(id), iterationType, number);
    }
}
