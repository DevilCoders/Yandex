package ru.yandex.ci.core.sandbox;

import java.util.List;
import java.util.stream.Collectors;

import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.client.sandbox.api.notification.NotificationSetting;
import ru.yandex.ci.client.sandbox.api.notification.NotificationStatus;
import ru.yandex.ci.client.sandbox.api.notification.NotificationTransport;
import ru.yandex.ci.core.config.a.model.SandboxNotificationConfig;
import ru.yandex.ci.ydb.Persisted;

@Deprecated // Must be removed in favor of simple 'SandboxNotificationConfig'
@Persisted
@Value
@Builder
public class NotificationSettingEntity {

    List<Status> statuses;
    List<String> recipients;
    Transport transport;

    public static NotificationSettingEntity fromConfig(SandboxNotificationConfig config) {
        return NotificationSettingEntity.builder()
                .recipients(List.copyOf(config.getRecipients()))
                .statuses(
                        config.getStatuses().stream()
                                .map(Enum::name)
                                .map(Status::valueOf)
                                .collect(Collectors.toList())
                ).transport(Transport.valueOf(config.getTransport().name()))
                .build();
    }

    public SandboxNotificationConfig toConfig() {
        return SandboxNotificationConfig.builder()
                .recipients(recipients)
                .transport(NotificationTransport.valueOf(transport.name()))
                .statuses(statuses.stream()
                        .map(Enum::name)
                        .map(NotificationStatus::valueOf)
                        .toList())
                .build();
    }

    public NotificationSetting toApi() {
        return new NotificationSetting(
                NotificationTransport.valueOf(transport.name()),
                statuses.stream()
                        .map(Enum::name)
                        .map(NotificationStatus::valueOf)
                        .collect(Collectors.toList()),
                List.copyOf(recipients)
        );
    }

    @Persisted
    public enum Transport {
        TELEGRAM,
        JABBER,
        EMAIL
    }

    @Persisted
    public enum Status {
        EXCEPTION,
        RELEASED,
        DRAFT,
        ENQUEUING,
        TIMEOUT,
        WAIT_MUTEX,
        FINISHING,
        WAIT_RES,
        WAIT_TASK,
        SUSPENDING,
        EXECUTING,
        STOPPING,
        EXPIRED,
        NOT_RELEASED,
        SUCCESS,
        ASSIGNED,
        STOPPED,
        TEMPORARY,
        NO_RES,
        DELETED,
        RELEASING,
        ENQUEUED,
        WAIT_TIME,
        FAILURE,
        WAIT_OUT,
        RESUMING,
        PREPARING,
        SUSPENDED,
    }

}
