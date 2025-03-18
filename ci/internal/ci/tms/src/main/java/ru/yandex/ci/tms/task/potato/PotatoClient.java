package ru.yandex.ci.tms.task.potato;

import java.util.Map;

import javax.annotation.Nullable;

import ru.yandex.ci.tms.task.potato.client.Status;

public interface PotatoClient {
    Map<String, Status> healthCheck(@Nullable String namespace);
}
