package ru.yandex.ci.client.staff;

import java.util.List;

import javax.annotation.Nullable;

import lombok.Value;

@Value
class GroupMembershipResponse {
    @Nullable
    List<StaffMembership> result;
}
