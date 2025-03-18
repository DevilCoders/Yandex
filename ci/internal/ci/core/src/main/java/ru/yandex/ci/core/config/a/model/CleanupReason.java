package ru.yandex.ci.core.config.a.model;

import java.util.Locale;

import com.fasterxml.jackson.annotation.JsonAlias;
import com.fasterxml.jackson.annotation.JsonProperty;

import ru.yandex.ci.ydb.Persisted;

@Persisted
public enum CleanupReason {
    @JsonProperty("new-diff-set")
    @JsonAlias("NEW_DIFF_SET")
    NEW_DIFF_SET,

    @JsonProperty("pr-merged")
    @JsonAlias("PR_MERGED")
    PR_MERGED,

    @JsonProperty("pr-discarded")
    @JsonAlias("PR_DISCARDED")
    PR_DISCARDED,

    @JsonProperty("finish")
    @JsonAlias("FINISH")
    FINISH;

    public String display() {
        return name().replace('_', ' ').toLowerCase(Locale.ROOT);
    }
}
