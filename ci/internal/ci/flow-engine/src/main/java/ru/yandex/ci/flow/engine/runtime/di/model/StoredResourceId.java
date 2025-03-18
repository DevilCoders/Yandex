package ru.yandex.ci.flow.engine.runtime.di.model;

import java.util.UUID;

import javax.annotation.Nonnull;

import lombok.AccessLevel;
import lombok.Getter;
import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value(staticConstructor = "of")
public class StoredResourceId {

    @Nonnull
    @Getter(AccessLevel.NONE)
    String value;

    public static StoredResourceId generate() {
        return new StoredResourceId(UUID.randomUUID().toString());
    }

    public String asString() {
        return value;
    }

}
