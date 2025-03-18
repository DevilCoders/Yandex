package ru.yandex.ci.client.arcanum;

import java.util.List;

import lombok.Value;

@Value
public class GroupsDto {
    List<Group> data;

    @Value
    public static class Group {
        String name;
        List<Member> members;
    }

    @Value
    public static class Member {
        String name;
    }
}
