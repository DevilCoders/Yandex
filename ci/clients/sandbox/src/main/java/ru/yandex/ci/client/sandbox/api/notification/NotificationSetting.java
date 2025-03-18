package ru.yandex.ci.client.sandbox.api.notification;

import java.util.List;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;
import org.apache.commons.lang3.builder.ReflectionToStringBuilder;
import org.apache.commons.lang3.builder.ToStringStyle;

public class NotificationSetting {

    private final NotificationTransport transport;
    private final List<NotificationStatus> statuses;
    private final List<String> recipients;

    @JsonCreator
    public NotificationSetting(
            @JsonProperty("transport") NotificationTransport transport,
            @JsonProperty("statuses") List<NotificationStatus> statuses,
            @JsonProperty("recipients") List<String> recipients
    ) {
        this.transport = transport;
        this.statuses = statuses;
        this.recipients = recipients;
    }

    @JsonProperty("transport")
    public NotificationTransport getTransport() {
        return transport;
    }

    @JsonProperty("statuses")
    public List<NotificationStatus> getStatuses() {
        return statuses;
    }

    @JsonProperty("recipients")
    public List<String> getRecipients() {
        return recipients;
    }

    @Override
    public String toString() {
        return ReflectionToStringBuilder.toString(this, ToStringStyle.SHORT_PREFIX_STYLE);
    }
}
