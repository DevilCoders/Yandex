package ru.yandex.ci.flow.engine.runtime.state;

import java.util.List;
import java.util.function.BiConsumer;
import java.util.function.Predicate;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;

import lombok.RequiredArgsConstructor;

import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.events.CancelGracefulDisablingEvent;
import ru.yandex.ci.flow.engine.runtime.events.FlowEvent;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;

@RequiredArgsConstructor
public class FlowStateService {
    @Nonnull
    private final FlowLaunchFactory flowLaunchFactory;
    @Nonnull
    private final StagedFlowStateUpdater stagedFlowStateUpdater;
    @Nonnull
    private final CasFlowStateUpdater casFlowStateUpdater;
    @Nonnull
    private final CiDb db;

    public FlowLaunchEntity activateLaunch(LaunchParameters parameters) {
        return activateLaunch(prepareLaunch(parameters));
    }

    public FlowLaunchEntity recalc(FlowLaunchId flowLaunchId, FlowEvent event) {
        FlowLaunchEntity flowLaunch = get(flowLaunchId);
        return getStateUpdater(flowLaunch).recalc(flowLaunchId, flowLaunch.getLaunchId(), event);
    }

    public void disableLaunchGracefully(FlowLaunchId flowLaunchId, boolean ignoreUninterruptableStage) {
        FlowLaunchEntity flowLaunch = get(flowLaunchId);
        getStateUpdater(flowLaunch)
                .disableLaunchGracefully(flowLaunchId, flowLaunch.getLaunchId(), ignoreUninterruptableStage);
    }

    public void cancelGracefulDisabling(FlowLaunchId flowLaunchId) {
        FlowLaunchEntity flowLaunch = get(flowLaunchId);
        getStateUpdater(flowLaunch).recalc(flowLaunchId, flowLaunch.getLaunchId(), new CancelGracefulDisablingEvent());
    }

    public void cleanupLaunch(FlowLaunchId flowLaunchId) {
        FlowLaunchEntity flowLaunch = get(flowLaunchId);
        getStateUpdater(flowLaunch).cleanupFlowLaunch(flowLaunchId, flowLaunch.getLaunchId());
    }

    public void disableJobsInLaunchGracefully(
            FlowLaunchId flowLaunchId,
            Predicate<JobState> predicate,
            boolean ignoreUninterruptableStage,
            boolean killJobs
    ) {
        update(
                flowLaunchId,
                (updater, launch) ->
                        updater.disableJobsInLaunchGracefully(
                                flowLaunchId,
                                launch.getLaunchId(),
                                getJobIds(launch, predicate),
                                ignoreUninterruptableStage,
                                killJobs
                        )
        );
    }

    //

    private FlowLaunchEntity activateLaunch(FlowLaunchEntity flowLaunch) {
        return getStateUpdater(flowLaunch).activateLaunch(flowLaunch);
    }

    private FlowLaunchEntity prepareLaunch(LaunchParameters parameters) {
        var flowLaunchId = FlowLaunchId.of(parameters.getLaunchId());
        var flowLaunchParameters = FlowLaunchParameters.builder()
                .flowLaunchId(flowLaunchId)
                .launchParameters(parameters)
                .build();

        return flowLaunchFactory.create(flowLaunchParameters);
    }

    private void update(FlowLaunchId flowLaunchId, BiConsumer<FlowStateUpdater, FlowLaunchEntity> action) {
        FlowLaunchEntity flowLaunch = get(flowLaunchId);
        action.accept(getStateUpdater(flowLaunch), flowLaunch);
    }

    private FlowStateUpdater getStateUpdater(FlowLaunchEntity flowLaunch) {
        return flowLaunch.isStaged() ? stagedFlowStateUpdater : casFlowStateUpdater;
    }

    private List<String> getJobIds(FlowLaunchEntity flowLaunch, Predicate<JobState> predicate) {
        return flowLaunch.getJobs().values().stream()
                .filter(predicate)
                .map(JobState::getJobId)
                .collect(Collectors.toList());
    }

    private FlowLaunchEntity get(FlowLaunchId flowLaunchId) {
        return db.currentOrReadOnly(() -> db.flowLaunch().get(flowLaunchId));
    }
}
