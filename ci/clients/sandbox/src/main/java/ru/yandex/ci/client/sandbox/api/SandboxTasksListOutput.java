package ru.yandex.ci.client.sandbox.api;

import java.util.List;

import lombok.Value;

@Value
public class SandboxTasksListOutput {
    List<SandboxTaskOutput> items;
    long total;
    int limit;
    int offset;
}
