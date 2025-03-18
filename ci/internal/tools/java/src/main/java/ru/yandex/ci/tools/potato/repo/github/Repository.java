package ru.yandex.ci.tools.potato.repo.github;

import lombok.Value;

@Value
public class Repository {
    String name;
    Owner owner;
}
