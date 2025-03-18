package ru.yandex.ci.core.launch;

import java.time.Instant;
import java.util.List;
import java.util.Objects;

import javax.annotation.Nullable;

import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import com.google.common.base.Strings;
import lombok.Value;

import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementId;
import ru.yandex.ci.core.pr.PullRequestVcsInfo;
import ru.yandex.ci.util.gson.GsonJacksonDeserializer;
import ru.yandex.ci.util.gson.GsonJacksonSerializer;
import ru.yandex.ci.ydb.Persisted;
import ru.yandex.lang.NonNullApi;

@Persisted
@Value
@NonNullApi
@JsonSerialize(using = GsonJacksonSerializer.class)
@JsonDeserialize(using = GsonJacksonDeserializer.class)
public class LaunchPullRequestInfo {
    long pullRequestId;
    long diffSetId;
    String author;
    @Nullable
    String summary;
    @Nullable
    String description;

    @Nullable
    ArcanumMergeRequirementId requirementId;    // can be null in autocheck stress tests

    PullRequestVcsInfo vcsInfo;
    List<String> pullRequestIssues;
    @Nullable
    List<String> pullRequestLabels;
    @Nullable
    Instant diffSetEventCreated;

    public String getAuthor() {
        return Strings.nullToEmpty(author);
    }

    public List<String> getPullRequestLabels() {
        return Objects.requireNonNullElse(pullRequestLabels, List.of());
    }
}
