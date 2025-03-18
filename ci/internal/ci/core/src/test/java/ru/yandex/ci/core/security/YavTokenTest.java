package ru.yandex.ci.core.security;

import java.time.Instant;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.test.TestData;

class YavTokenTest {

    @Test
    void noTokenInToStringTest() {
        String delegationToken = "TOKEN";

        YavToken yavToken = YavToken.builder()
                .id(YavToken.Id.of("abc"))
                .configPath(TestData.CONFIG_PATH_ABC.toString())
                .secretUuid("sec-42")
                .token(delegationToken)
                .delegationCommitId("d39bef21436e33e8ee71fb17e3350a5042b3a274")
                .delegatedBy("andreevdm")
                .abcService("ci")
                .created(Instant.now())
                .build();

        Assertions.assertFalse(yavToken.toString().contains(delegationToken));
    }
}
