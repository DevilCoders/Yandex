package ru.yandex.ci.client.teamcity;

import lombok.Value;

@Value
public class TeamcityBuildType {
    String id;
    String projectId;
}
