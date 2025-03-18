package ru.yandex.ci.storage.core.db.model.check_merge_requirements;

import java.time.LocalTime;

import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@Value
@Persisted
public class MergeInterval {
    LocalTime from;
    LocalTime to;
}
