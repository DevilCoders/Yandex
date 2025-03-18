package ru.yandex.ci.storage.tests.yt;

import java.util.concurrent.CompletableFuture;
import java.util.function.Consumer;

import lombok.AllArgsConstructor;

import ru.yandex.ci.storage.core.yt.YtClientFactory;
import ru.yandex.yt.ytclient.proxy.MockYtClient;
import ru.yandex.yt.ytclient.proxy.TransactionalClient;

@AllArgsConstructor
public class YtClientFactoryEmptyImpl implements YtClientFactory {
    @Override
    public void execute(Consumer<TransactionalClient> callback) {
        var client = new MockYtClient("hahn");

        for (int i = 0; i < 4; ++i) {
            client.mockMethod("existsNode", () -> CompletableFuture.completedFuture(true));
            client.mockMethod("writeTable", () -> CompletableFuture.completedFuture(new EmptyTableWriter()));
        }

        callback.accept(client);
    }
}
