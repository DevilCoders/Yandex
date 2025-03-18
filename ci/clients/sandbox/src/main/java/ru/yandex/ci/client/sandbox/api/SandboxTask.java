
package ru.yandex.ci.client.sandbox.api;

import java.io.Serializable;
import java.time.Duration;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import javax.annotation.Nullable;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.client.sandbox.api.notification.NotificationSetting;

@Value
@Builder
@AllArgsConstructor
public class SandboxTask implements Serializable {
    String id;
    String type;
    String templateAlias;

    String author;
    String owner;

    @lombok.Builder.Default
    List<SandboxCustomField> customFields = new ArrayList<>();

    String description;

    @lombok.Builder.Default
    Map<String, Object> context = new HashMap<>();
    SandboxTaskPriority priority;
    List<String> tags;
    List<String> hints;

    List<NotificationSetting> notifications;
    SandboxTaskRequirements requirements;
    @Nullable
    Long killTimeout;

    Long tasksArchiveResource;

    public static class Builder {
        public Builder killTimeoutDuration(@Nullable Duration duration) {
            if (duration == null) {
                killTimeout(null);
            } else {
                killTimeout(duration.toSeconds());
            }
            return this;
        }
    }
}
