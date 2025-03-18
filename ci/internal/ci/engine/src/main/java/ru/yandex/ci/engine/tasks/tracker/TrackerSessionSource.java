package ru.yandex.ci.engine.tasks.tracker;

import javax.annotation.Nonnull;

import lombok.AllArgsConstructor;
import lombok.Getter;
import yav_service.YavOuterClass;

import ru.yandex.ci.client.tracker.TrackerClient;
import ru.yandex.ci.core.security.YavToken;
import ru.yandex.ci.engine.flow.SecurityAccessService;
import ru.yandex.startrek.client.Session;

@AllArgsConstructor
public class TrackerSessionSource {

    @Nonnull
    private final TrackerClient trackerClient;

    @Nonnull
    private final SecurityAccessService securityAccessService;

    @Getter
    @Nonnull
    private final String trackerUrl;

    public Session getSession(YavOuterClass.YavSecretSpec spec, YavToken.Id tokenId) {
        if (spec.getKey().isEmpty()) {
            throw new RuntimeException("Key for YAV token is not provided, check your settings in `config.secret.key`");
        }
        var secret = securityAccessService.getYavSecret(tokenId);
        var token = secret.getValueByKey(spec.getKey())
                .orElseThrow(() -> new RuntimeException(
                        "Unable to find key [%s] in YAV token, check your settings in `config.secret.key`"
                                .formatted(spec.getKey())));

        return trackerClient.getSession(token);
    }

}
