package ru.yandex.ci.engine.launch;

import java.time.Clock;
import java.time.Duration;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.common.bazinga.AbstractOnetimeTask;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.state.calculator.FlowLaunchMutexManager;
import ru.yandex.commune.bazinga.BazingaTryLaterException;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.misc.bender.annotation.BenderBindAllFields;

@Slf4j
public class LaunchCancelTask extends AbstractOnetimeTask<LaunchCancelTask.Params> {

    private CiMainDb db;
    private FlowLaunchService flowLaunchService;
    private LaunchStateSynchronizer launchStateSynchronizer;
    private Clock clock;
    private FlowLaunchMutexManager flowLaunchMutexManager;

    public LaunchCancelTask(
            CiMainDb db,
            FlowLaunchService flowLaunchService,
            LaunchStateSynchronizer launchStateSynchronizer,
            Clock clock,
            FlowLaunchMutexManager flowLaunchMutexManager
    ) {
        super(LaunchCancelTask.Params.class);
        this.db = db;
        this.flowLaunchService = flowLaunchService;
        this.launchStateSynchronizer = launchStateSynchronizer;
        this.clock = clock;
        this.flowLaunchMutexManager = flowLaunchMutexManager;
    }


    public LaunchCancelTask(LaunchCancelTask copyFrom) {
        this(copyFrom.db, copyFrom.flowLaunchService, copyFrom.launchStateSynchronizer, copyFrom.clock,
                copyFrom.flowLaunchMutexManager);
    }

    public LaunchCancelTask(LaunchId launchId) {
        super(new Params(launchId));
    }

    @Override
    protected void execute(Params params, ExecutionContext context) {
        log.info("Cancelling {}", params.getLaunchId());
        flowLaunchMutexManager.acquireAndRun(
                params.getLaunchId(),
                () -> doExecute(params)
        );
        log.info("Cancelling executed {}", params.getLaunchId());
    }

    private void doExecute(Params params) {
        db.currentOrTx(() -> {
            Launch launch = db.launches().get(params.getLaunchId());

            LaunchState.Status status = launch.getStatus();

            if (status.isTerminal()) {
                log.info("launch {} is already in terminal status {}, nothing to cancel", launch.getLaunchId(), status);
                return;
            }

            if (status != LaunchState.Status.CANCELLING) {
                log.info("Launch {} is not marked cancelled yet, it is {}, postpone flow cancelling",
                        launch.getLaunchId(), status);
                throw new BazingaTryLaterException(org.joda.time.Duration.standardSeconds(5));
            }

            launch.getState().getFlowLaunchId()
                    .map(FlowLaunchId::of)
                    .ifPresentOrElse(
                            id -> {
                                flowLaunchService.cancelFlow(id);
                                log.info("Flow {} cancelled", id);
                            },
                            () -> {
                                launchStateSynchronizer.stateUpdated(
                                        launch,
                                        new LaunchState(
                                                launch.getFlowLaunchId(),
                                                launch.getStarted(),
                                                clock.instant(),
                                                LaunchState.Status.CANCELED,
                                                launch.getStatusText()),
                                        false,
                                        null);
                                log.info("Launch {} doesn't have flow launch, launch marked cancelled", launch.getId());
                            }
                    );
        });
    }

    @Override
    public Duration getTimeout() {
        return Duration.ofMinutes(1);
    }

    @BenderBindAllFields
    public static class Params {
        private final String ciProcessId;
        private final int number;

        public Params(LaunchId launchId) {
            this.ciProcessId = launchId.getProcessId().asString();
            this.number = launchId.getNumber();
        }

        public LaunchId getLaunchId() {
            return LaunchId.of(CiProcessId.ofString(ciProcessId), number);
        }

    }
}
