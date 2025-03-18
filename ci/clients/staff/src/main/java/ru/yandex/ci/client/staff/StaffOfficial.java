package ru.yandex.ci.client.staff;

import lombok.Value;

@Value
public class StaffOfficial {
    String affiliation;
    boolean isRobot;

    public boolean isOutStaff() {
        return affiliation.equals("external") && !isRobot;
    }
}
