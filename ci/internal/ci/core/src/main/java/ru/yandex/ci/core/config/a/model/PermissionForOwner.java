package ru.yandex.ci.core.config.a.model;

import com.fasterxml.jackson.annotation.JsonAlias;
import com.fasterxml.jackson.annotation.JsonProperty;

import ru.yandex.ci.ydb.Persisted;

@Persisted
public enum PermissionForOwner {
    @JsonProperty("pr")
    @JsonAlias("PR")
    PR,

    @JsonProperty("commit")
    @JsonAlias("COMMIT")
    COMMIT,

    @JsonProperty("release")
    @JsonAlias("RELEASE")
    RELEASE;
}
