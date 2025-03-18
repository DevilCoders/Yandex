package ru.yandex.ci.tools.potato.repo.arcadia;

import java.util.List;

import lombok.Value;

@Value
public class ArcadiaSearchResults {
    List<Item> results;
    Stats stats;
}
