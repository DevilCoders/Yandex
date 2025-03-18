package ru.yandex.ci.storage.core.db.model.check;

import javax.annotation.Nullable;

import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
public class SuspiciousAlert {
    @Nullable
    String id;
    String message;

    public String getId() {
        return id == null ? "" : id;
    }
}
