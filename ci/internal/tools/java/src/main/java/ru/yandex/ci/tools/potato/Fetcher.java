package ru.yandex.ci.tools.potato;

import java.util.List;

public interface Fetcher {
    List<ConfigRef> findAll();

    String getContent(String path);

    default String getDefaultBranch(String project, String repo) {
        throw new UnsupportedOperationException();
    }
}
