package ru.yandex.ci.storage.core.large;

import lombok.Value;

import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.misc.bender.annotation.BenderBindAllFields;

@Value
@BenderBindAllFields
public class BazingaCheckId {
    Long id;

    public BazingaCheckId(CheckEntity.Id id) {
        this.id = id.getId();
    }

    public CheckEntity.Id getCheckId() {
        return CheckEntity.Id.of(id);
    }
}
