package ru.yandex.ci.tools.potato.repo.arcadia;

import lombok.Value;

@Value
public class Item {
    String path;
    boolean isDir;
}
