package ru.yandex.ci.tools.potato.repo.github;

import lombok.Value;

@Value
public class Item {
    String htmlUrl;
    String path;
    Repository repository;
}

