package ru.yandex.ci.common.bazinga;

import ru.yandex.commune.bazinga.scheduler.ActiveUniqueIdentifierConverter;

public abstract class StringUniqueIdentifierConverter<Parameters> implements
        ActiveUniqueIdentifierConverter<Parameters, StringUniqueIdentifier> {

    @Override
    public Class<StringUniqueIdentifier> getActiveUniqueIdentifierClass() {
        return StringUniqueIdentifier.class;
    }

    @Override
    public StringUniqueIdentifier convert(Parameters parameters) {
        return new StringUniqueIdentifier(convertToString(parameters));
    }

    protected abstract String convertToString(Parameters parameters);
}
