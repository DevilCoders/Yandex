package ru.yandex.ci.engine.flow;

import javax.annotation.Nonnull;

import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.client.yav.YavClient;
import ru.yandex.ci.client.yav.model.YavSecret;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.security.YavToken;

@Slf4j
@RequiredArgsConstructor
public class SecurityAccessService {

    @Nonnull
    private final YavClient yavClient;

    @Nonnull
    private final CiMainDb db;

    public YavSecret getYavSecret(YavToken.Id yavTokenUid) {
        log.info("Get Yav secret using token: {}", yavTokenUid);
        var yavToken = db.currentOrReadOnly(() ->
                db.yavTokensTable().get(yavTokenUid));
        YavSecret yavSecret = getYavSecret(yavToken.getToken(), yavToken.getConfigPath());
        yavSecret.setSecret(yavToken.getSecretUuid());
        return yavSecret;
    }

    private YavSecret getYavSecret(String delegatingToken, String signature) {
        return yavClient.getSecretByDelegatingToken(delegatingToken, null, signature);
    }

}
