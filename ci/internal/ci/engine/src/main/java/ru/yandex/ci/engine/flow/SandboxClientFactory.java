package ru.yandex.ci.engine.flow;

import ru.yandex.ci.client.sandbox.SandboxClient;

public interface SandboxClientFactory {

    SandboxClient create(String oauthToken);
}
