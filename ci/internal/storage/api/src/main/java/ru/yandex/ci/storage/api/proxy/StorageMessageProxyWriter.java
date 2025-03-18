package ru.yandex.ci.storage.api.proxy;

import java.util.List;

import ru.yandex.ci.storage.core.MainStreamMessages;

public interface StorageMessageProxyWriter {
    void writeTasks(List<MainStreamMessages.MainStreamMessage> messages);
}
