package ru.yandex.ci.storage.tests;

import java.util.Set;
import java.util.concurrent.CompletableFuture;
import java.util.stream.Collectors;

import ru.yandex.ci.ayamler.Ayamler;
import ru.yandex.ci.client.ayamler.AYamlerClient;
import ru.yandex.ci.client.ayamler.StrongModeRequest;

public class TestAYamlerClientImpl implements AYamlerClient {
    private boolean enabled;

    public void enableStrongMode() {
        this.enabled = true;
    }

    public void disableStrongMode() {
        this.enabled = false;
    }

    @Override
    public CompletableFuture<Ayamler.GetStrongModeBatchResponse> getStrongMode(Set<StrongModeRequest> requests) {
        var response = requests.stream()
                .map(x -> Ayamler.StrongMode.newBuilder()
                        .setPath(x.getPath())
                        .setLogin(x.getLogin())
                        .setRevision(x.getRevision())
                        .setIsOwner(x.getLogin().equals("owner"))
                        .setStatus(this.enabled ? Ayamler.StrongModeStatus.ON : Ayamler.StrongModeStatus.OFF)
                        .setAyaml(
                                Ayamler.AYaml.newBuilder()
                                        .setService("ci")
                                        .build()
                        )
                        .build()
                )
                .collect(Collectors.toList());

        return CompletableFuture.completedFuture(
                Ayamler.GetStrongModeBatchResponse.newBuilder()
                        .addAllStrongMode(response)
                        .build()
        );
    }
}
