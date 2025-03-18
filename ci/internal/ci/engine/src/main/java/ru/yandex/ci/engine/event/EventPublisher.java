package ru.yandex.ci.engine.event;

import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.common.temporal.TemporalService;
import ru.yandex.ci.engine.event.proto.CiEvent;
import ru.yandex.commune.bazinga.BazingaTaskManager;

@Slf4j
@RequiredArgsConstructor
public class EventPublisher {

    private final BazingaTaskManager bazingaTaskManager;

    private final TemporalService temporalService;

    public void publish(LaunchEvent event) {
        byte[] data = event.getData();
        if (data.length > 0) {
            var launchId = event.getLaunchId();
            log.info("Sending LaunchEventTask: {} with status {}", launchId, event.getLaunchStatus());
            LaunchEventTask task = new LaunchEventTask(launchId, event.getLaunchStatus(), data);
            bazingaTaskManager.schedule(task);
        }
    }

    public void publish(CiEvent event) {
        var payload = CiEventPayload.ofEvent(event);
        log.info("Sending CiEvent {}", payload.getTemporalWorkflowId());
        temporalService.startInTx(CiEventWorkflow.class, wf -> wf::send, payload);
    }
}
