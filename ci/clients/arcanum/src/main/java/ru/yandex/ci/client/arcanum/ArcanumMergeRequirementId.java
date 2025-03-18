package ru.yandex.ci.client.arcanum;

import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value(staticConstructor = "of")
public class ArcanumMergeRequirementId {
    String system;
    String type;
}
