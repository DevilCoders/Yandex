package ru.yandex.ci.client.arcanum.util;

import lombok.Value;

@Value(staticConstructor = "of")
public class RevisionNumberPullRequestIdPair {
    long revisionNumber;
    long pullRequestId;
}
