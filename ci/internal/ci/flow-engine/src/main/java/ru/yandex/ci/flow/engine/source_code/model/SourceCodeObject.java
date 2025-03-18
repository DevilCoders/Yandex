package ru.yandex.ci.flow.engine.source_code.model;

import java.util.UUID;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import ru.yandex.ci.core.common.SourceCodeEntity;

/**
 * Описание сущности типа SourceCodeEntity.
 * Типы полей, заголовок и прочие опции. См. наследников.
 */
public class SourceCodeObject<T extends SourceCodeEntity> {
    protected final UUID id;
    protected final Class<? extends T> clazz;
    protected final SourceCodeObjectType type;

    public SourceCodeObject(@Nonnull UUID id,
                            @Nullable Class<? extends T> clazz,
                            @Nonnull SourceCodeObjectType type) {
        this.id = id;
        this.clazz = clazz;
        this.type = type;
    }

    public UUID getId() {
        return id;
    }

    @Nullable
    public Class<? extends T> getClazz() {
        return clazz;
    }

    public SourceCodeObjectType getType() {
        return type;
    }
}
