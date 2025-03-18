package ru.yandex.ci.storage.core.db.model.check_task;

import java.util.Map;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@Value
@Builder
@Persisted
@AllArgsConstructor
public class NativeSpecification {
    Map<String, Target> targets;

    @Value
    @Persisted
    @lombok.Builder
    @AllArgsConstructor
    public static class Target {
        String hid;
    }
}
