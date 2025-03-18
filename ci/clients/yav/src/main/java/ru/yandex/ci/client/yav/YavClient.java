package ru.yandex.ci.client.yav;

import ru.yandex.ci.client.yav.model.DelegatingTokenResponse;
import ru.yandex.ci.client.yav.model.YavSecret;

public interface YavClient {

    YavSecret getSecretByDelegatingToken(
            String token,
            String secretVersion,
            String signature
    );

    DelegatingTokenResponse createDelegatingToken(
            String tvmUserTicket,
            String secretUuid,
            String signature,
            String comment
    );

    String getReaders(String secretUuid);
}
