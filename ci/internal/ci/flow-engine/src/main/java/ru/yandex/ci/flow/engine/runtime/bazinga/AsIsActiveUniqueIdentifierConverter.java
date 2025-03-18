package ru.yandex.ci.flow.engine.runtime.bazinga;

import ru.yandex.commune.bazinga.scheduler.ActiveUniqueIdentifierConverter;

public class AsIsActiveUniqueIdentifierConverter<TSource> implements ActiveUniqueIdentifierConverter<TSource, TSource> {
    private final Class<TSource> clazz;

    public AsIsActiveUniqueIdentifierConverter(Class<TSource> clazz) {
        this.clazz = clazz;
    }

    @Override
    public Class<TSource> getActiveUniqueIdentifierClass() {
        return clazz;
    }

    @Override
    public TSource convert(TSource parameters) {
        return parameters;
    }
}
