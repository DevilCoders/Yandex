package ru.yandex.ci.engine.event;

import io.temporal.activity.ActivityInterface;
import io.temporal.activity.ActivityMethod;

@ActivityInterface
public interface CiEventActivity {
    @ActivityMethod
    void send(CiEventPayload id);
}
