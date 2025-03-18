package ru.yandex.ci.client.yav.model;

import java.util.Objects;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonAlias;
import com.google.common.base.MoreObjects;
import com.google.errorprone.annotations.ForOverride;

public class YavResponse {
    @Nullable
    private final Status status;
    @Nullable
    private final String code;
    @Nullable
    private final String message;

    public YavResponse(@Nullable Status status,
                       @Nullable String code,
                       @Nullable String message) {
        this.status = status;
        this.code = code;
        this.message = message;
    }

    @Nullable
    public Status getStatus() {
        return status;
    }

    @Nullable
    public String getCode() {
        return code;
    }

    @Nullable
    public String getMessage() {
        return message;
    }

    @Override
    public String toString() {
        return toStringHelper().toString();
    }

    @ForOverride
    protected MoreObjects.ToStringHelper toStringHelper() {
        return MoreObjects.toStringHelper(this)
                .add("status", status)
                .add("code", code)
                .add("message", message);
    }

    @Override
    public int hashCode() {
        return Objects.hash(super.hashCode(), status, code, message);
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }

        if (!(obj instanceof YavResponse)) {
            return false;
        }

        YavResponse that = (YavResponse) obj;

        return Objects.equals(status, that.status) &&
                Objects.equals(code, that.code) &&
                Objects.equals(message, that.message);
    }

    public enum Status {
        @JsonAlias("ok")
        OK,
        @JsonAlias("warning")
        WARNING,
        @JsonAlias("error")
        ERROR
    }
}
