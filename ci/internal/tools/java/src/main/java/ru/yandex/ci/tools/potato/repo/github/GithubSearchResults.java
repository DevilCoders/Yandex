package ru.yandex.ci.tools.potato.repo.github;

import java.util.List;

import lombok.Value;

@Value
public class GithubSearchResults {
    boolean incompleteResults;
    List<Item> items;
}
