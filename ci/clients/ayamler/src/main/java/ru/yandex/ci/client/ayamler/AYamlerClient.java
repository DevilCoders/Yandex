package ru.yandex.ci.client.ayamler;

import java.util.Set;
import java.util.concurrent.CompletableFuture;

import ru.yandex.ci.ayamler.Ayamler;

public interface AYamlerClient {
    CompletableFuture<Ayamler.GetStrongModeBatchResponse> getStrongMode(Set<StrongModeRequest> requests);
}
