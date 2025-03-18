package ru.yandex.ci.core.tasklet;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@Value
@Persisted
@JsonIgnoreProperties(ignoreUnknown = true)
@Builder
public class Features {
    boolean consumesSecretId;

    public static Features empty() {
        return new Features(false);
    }
}
