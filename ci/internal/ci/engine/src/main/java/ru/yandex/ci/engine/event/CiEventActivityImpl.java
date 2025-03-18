package ru.yandex.ci.engine.event;

import java.util.concurrent.ExecutionException;

import lombok.RequiredArgsConstructor;

import ru.yandex.ci.common.temporal.config.ActivityImplementation;
import ru.yandex.ci.core.logbroker.LogbrokerWriter;

@RequiredArgsConstructor
public class CiEventActivityImpl implements CiEventActivity, ActivityImplementation {

    private final LogbrokerWriter logbrokerCiEventWriter;

    @Override
    public void send(CiEventPayload id) {
        try {
            logbrokerCiEventWriter.write(id.getPayload().toByteArray()).get();
        } catch (InterruptedException | ExecutionException e) {
            throw new RuntimeException(e);
        }
    }
}
