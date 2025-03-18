package ru.yandex.ci.client.staff;

import lombok.Value;

@Value
public class StaffPerson {
    String login;
    StaffOfficial official;
}
