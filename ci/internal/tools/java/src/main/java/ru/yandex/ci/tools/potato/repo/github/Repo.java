package ru.yandex.ci.tools.potato.repo.github;

import lombok.Value;

@Value
public class Repo {
    String defaultBranch;
    boolean archived;
}
