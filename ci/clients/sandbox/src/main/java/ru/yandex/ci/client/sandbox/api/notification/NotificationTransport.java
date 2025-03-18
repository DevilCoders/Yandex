package ru.yandex.ci.client.sandbox.api.notification;

import com.fasterxml.jackson.annotation.JsonAlias;
import com.fasterxml.jackson.annotation.JsonProperty;

import ru.yandex.ci.ydb.Persisted;

@Persisted
public enum NotificationTransport {
    @JsonProperty("telegram")
    @JsonAlias("TELEGRAM")
    TELEGRAM,
    @JsonProperty("jabber")
    @JsonAlias("JABBER")
    JABBER,
    @JsonProperty("email")
    @JsonAlias("EMAIL")
    EMAIL,
    @JsonProperty("q")
    @JsonAlias("Q")
    Q
}
