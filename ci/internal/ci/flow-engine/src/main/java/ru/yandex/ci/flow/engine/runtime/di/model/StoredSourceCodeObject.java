package ru.yandex.ci.flow.engine.runtime.di.model;

import java.util.UUID;

import javax.annotation.Nonnull;

import com.google.gson.JsonObject;
import lombok.AllArgsConstructor;
import lombok.Getter;
import lombok.ToString;

import ru.yandex.ci.core.common.SourceCodeEntity;

@Getter
@ToString
@AllArgsConstructor
public class StoredSourceCodeObject<T extends SourceCodeEntity> implements Comparable<StoredSourceCodeObject<T>> {

    @Nonnull
    private final UUID sourceCodeId;

    @Nonnull
    private final JsonObject object;

    @Override
    public int compareTo(StoredSourceCodeObject<T> o) {
        return this.sourceCodeId.compareTo(o.sourceCodeId);
    }
}
