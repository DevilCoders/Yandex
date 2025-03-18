package ru.yandex.ci.storage.core.yt;

import java.util.function.Consumer;

import ru.yandex.yt.ytclient.proxy.TransactionalClient;

public interface YtClientFactory {
    void execute(Consumer<TransactionalClient> callback);
}
