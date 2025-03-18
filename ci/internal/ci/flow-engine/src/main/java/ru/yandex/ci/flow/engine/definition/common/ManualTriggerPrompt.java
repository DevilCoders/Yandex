package ru.yandex.ci.flow.engine.definition.common;

import javax.annotation.Nullable;

import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
public class ManualTriggerPrompt {

    @Nullable
    String question;
}
