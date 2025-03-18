package ru.yandex.ci.core.db.table;

import java.util.Set;

import lombok.Value;

@Value(staticConstructor = "of")
public class ProjectFilter {
    String filter;
    Set<String> includeProjects;
}
