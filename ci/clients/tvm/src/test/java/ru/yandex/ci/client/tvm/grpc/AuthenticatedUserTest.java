package ru.yandex.ci.client.tvm.grpc;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

class AuthenticatedUserTest {

    @Test
    void noSecurityDataInToString() {
        String ticket = "sec-abc";

        AuthenticatedUser user = new AuthenticatedUser(ticket, "firov", "localhost");
        Assertions.assertFalse(
                user.toString().contains(ticket)
        );

    }
}
