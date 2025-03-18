package ru.yandex.ci.client.tvm.grpc;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.ToString;
import lombok.Value;

@Value
public class AuthenticatedUser {
    // Could be null if accessed by Service-Ticket or OAuth token
    @ToString.Exclude
    @Nullable
    String tvmUserTicket;

    @Nonnull
    String login;

    @Nonnull
    String ipAddress;
}
