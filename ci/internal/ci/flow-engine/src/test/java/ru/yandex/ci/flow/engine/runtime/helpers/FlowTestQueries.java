package ru.yandex.ci.flow.engine.runtime.helpers;

import java.time.Duration;
import java.time.Instant;
import java.util.Optional;

import javax.annotation.Nonnull;

import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.di.model.ResourceRefContainer;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResourceContainer;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;

@Slf4j
@RequiredArgsConstructor
public class FlowTestQueries {

    private static final Duration MAX_WAIT_DURATION = Duration.ofSeconds(5);

    @Nonnull
    private final CiDb db;

    public StoredResourceContainer getProducedResources(FlowLaunchId flowLaunchId, String jobId) {
        JobLaunch lastLaunch;

        Instant waitDeadline = Instant.now().plus(MAX_WAIT_DURATION);
        while (true) {
            lastLaunch = getJobLastLaunch(flowLaunchId, jobId);
            if (lastLaunch.getLastStatusChange().getType().isFinished()
                    || Instant.now().isAfter(waitDeadline)) {
                break;
            }
            log.info("Waiting finished state of job {} in flowLaunchId {}", jobId, flowLaunchId);
            sleepSilently();
        }
        return loadResources(lastLaunch.getProducedResources());
    }

    public StoredResourceContainer getConsumedResources(FlowLaunchId flowLaunchId, String jobId) {
        JobLaunch lastLaunch = getJobLastLaunch(flowLaunchId, jobId);

        ResourceRefContainer producedResourceRefs = lastLaunch.getConsumedResources();

        return loadResources(producedResourceRefs);
    }

    public JobLaunch getJobLastLaunch(FlowLaunchId flowLaunchId, String jobId) {
        FlowLaunchEntity flowLaunch = getFlowLaunch(flowLaunchId);
        return flowLaunch.getJobState(jobId).getLastLaunch();
    }

    public FlowLaunchEntity getFlowLaunch(FlowLaunchId flowLaunchId) {
        return db.currentOrReadOnly(() ->
                db.flowLaunch().get(flowLaunchId));
    }

    public Optional<FlowLaunchEntity> getFlowLaunchOptional(FlowLaunchId flowLaunchId) {
        return db.currentOrReadOnly(() ->
                db.flowLaunch().findOptional(flowLaunchId));
    }

    private StoredResourceContainer loadResources(ResourceRefContainer ref) {
        return db.currentOrReadOnly(() ->
                db.resources().loadResources(ref));
    }

    private static void sleepSilently() {
        try {
            Thread.sleep(100);
        } catch (InterruptedException e) {
            // noop
        }
    }
}
