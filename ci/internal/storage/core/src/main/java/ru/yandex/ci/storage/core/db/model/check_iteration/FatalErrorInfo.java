package ru.yandex.ci.storage.core.db.model.check_iteration;

import java.util.Objects;

import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@Value
@Persisted
@Builder(buildMethodName = "buildInternal")
public class FatalErrorInfo {
    public static final FatalErrorInfo EMPTY = FatalErrorInfo.builder().build();

    String message;
    String details;
    String sandboxTaskId;

    public static class Builder {
        public FatalErrorInfo build() {
            this.message(Objects.requireNonNullElse(this.message, ""));
            this.details(Objects.requireNonNullElse(this.details, ""));
            this.sandboxTaskId(Objects.requireNonNullElse(this.sandboxTaskId, ""));
            return buildInternal();
        }
    }
}
