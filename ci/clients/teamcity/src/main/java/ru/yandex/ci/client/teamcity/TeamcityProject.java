package ru.yandex.ci.client.teamcity;

import lombok.Value;

@Value
public class TeamcityProject {
    String id;
    String name;
    String description;
    String webUrl;
}
