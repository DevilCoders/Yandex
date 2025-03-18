package ru.yandex.ci.client.testenv.model;

import lombok.Value;

@Value
public class TestenvListProject {
    String name;
    String shard;
    String projectType;
    String taskOwner;
    TestenvProject.Status status;
}
