package ru.yandex.ci.storage.core.yt.impl;

import java.util.function.Consumer;

import lombok.AllArgsConstructor;

import ru.yandex.ci.storage.core.yt.YtClientFactory;
import ru.yandex.yt.ytclient.proxy.RetryPolicy;
import ru.yandex.yt.ytclient.proxy.TransactionalClient;
import ru.yandex.yt.ytclient.proxy.YtClient;
import ru.yandex.yt.ytclient.rpc.RpcCredentials;
import ru.yandex.yt.ytclient.rpc.RpcOptions;

@AllArgsConstructor
public class YtClientFactoryImpl implements YtClientFactory {

    private final RpcCredentials rpcCredentials;

    @Override
    public void execute(Consumer<TransactionalClient> callback) {
        var client = YtClient.builder()
                .setCluster("hahn")
                .setRpcCredentials(rpcCredentials)
                .setRpcOptions(new RpcOptions().setRetryPolicyFactory(RetryPolicy::defaultPolicy))
                .build();

        try (client) {
            callback.accept(client);
        }
    }
}
