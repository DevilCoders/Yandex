package ru.yandex.ci.engine.flow;

import javax.annotation.Nonnull;

import lombok.AllArgsConstructor;

import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.OAuthProvider;
import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.client.sandbox.SandboxClientImpl;
import ru.yandex.ci.client.sandbox.SandboxClientProperties;

@AllArgsConstructor
public class SandboxClientFactoryImpl implements SandboxClientFactory {

    @Nonnull
    private final String apiUrl;

    @Nonnull
    private final String apiV2Url;

    @Nonnull
    private final HttpClientProperties httpClientProperties;

    @Override
    public SandboxClient create(String oauthToken) {
        return SandboxClientImpl.create(
                SandboxClientProperties.builder()
                        .sandboxApiUrl(apiUrl)
                        .sandboxApiV2Url(apiV2Url)
                        .httpClientProperties(httpClientProperties.withAuthProvider(new OAuthProvider(oauthToken)))
                        .build()
        );
    }
}
