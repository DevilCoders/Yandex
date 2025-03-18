package ru.yandex.ci.core.pr;

import javax.annotation.Nullable;

import lombok.AllArgsConstructor;
import lombok.Value;

import ru.yandex.ci.core.arc.ArcBranch;

@Value
@AllArgsConstructor
public class PullRequest {
    long id;
    String author;
    String summary;
    @Nullable
    String description;
    @Nullable
    ArcBranch upstreamBranch;

    public PullRequest(long id, String author, String summary, @Nullable String description) {
        this(id, author, summary, description, ArcBranch.trunk());
    }
}
