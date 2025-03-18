package ru.yandex.ci.client.sandbox.api.notification;

import com.fasterxml.jackson.annotation.JsonProperty;

import ru.yandex.ci.ydb.Persisted;

@Persisted
public enum NotificationStatus {
    @JsonProperty("EXCEPTION")
    EXCEPTION,
    @JsonProperty("RELEASED")
    RELEASED,
    @JsonProperty("DRAFT")
    DRAFT,
    @JsonProperty("ENQUEUING")
    ENQUEUING,
    @JsonProperty("TIMEOUT")
    TIMEOUT,
    @JsonProperty("WAIT_MUTEX")
    WAIT_MUTEX,
    @JsonProperty("FINISHING")
    FINISHING,
    @JsonProperty("WAIT_RES")
    WAIT_RES,
    @JsonProperty("WAIT_TASK")
    WAIT_TASK,
    @JsonProperty("SUSPENDING")
    SUSPENDING,
    @JsonProperty("EXECUTING")
    EXECUTING,
    @JsonProperty("STOPPING")
    STOPPING,
    @JsonProperty("EXPIRED")
    EXPIRED,
    @JsonProperty("NOT_RELEASED")
    NOT_RELEASED,
    @JsonProperty("SUCCESS")
    SUCCESS,
    @JsonProperty("ASSIGNED")
    ASSIGNED,
    @JsonProperty("STOPPED")
    STOPPED,
    @JsonProperty("TEMPORARY")
    TEMPORARY,
    @JsonProperty("NO_RES")
    NO_RES,
    @JsonProperty("DELETED")
    DELETED,
    @JsonProperty("RELEASING")
    RELEASING,
    @JsonProperty("ENQUEUED")
    ENQUEUED,
    @JsonProperty("WAIT_TIME")
    WAIT_TIME,
    @JsonProperty("FAILURE")
    FAILURE,
    @JsonProperty("WAIT_OUT")
    WAIT_OUT,
    @JsonProperty("RESUMING")
    RESUMING,
    @JsonProperty("PREPARING")
    PREPARING,
    @JsonProperty("SUSPENDED")
    SUSPENDED,
}
