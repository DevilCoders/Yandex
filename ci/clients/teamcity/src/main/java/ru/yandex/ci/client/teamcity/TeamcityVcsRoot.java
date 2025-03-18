package ru.yandex.ci.client.teamcity;

import lombok.Value;

@Value
public class TeamcityVcsRoot {
    String id;
    TeamcityProject project;
}
