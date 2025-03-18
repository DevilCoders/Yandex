package ru.yandex.ci.engine.launch;

import java.time.Duration;
import java.util.Optional;

import com.google.common.base.Preconditions;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.ci.common.bazinga.AbstractOnetimeTask;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.VirtualCiProcessId;
import ru.yandex.ci.core.config.a.validation.AYamlValidationException;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.state.calculator.FlowLaunchMutexManager;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.misc.bender.annotation.BenderBindAllFields;

public class LaunchStartTask extends AbstractOnetimeTask<LaunchStartTask.Params> {

    private static final Logger log = LoggerFactory.getLogger(LaunchStartTask.class);

    private CiMainDb db;
    private FlowLaunchService flowLaunchService;
    private ConfigurationService configurationService;
    private FlowLaunchMutexManager flowLaunchMutexManager;

    public LaunchStartTask(CiMainDb db,
                           FlowLaunchService flowLaunchService,
                           ConfigurationService configurationService,
                           FlowLaunchMutexManager flowLaunchMutexManager) {
        super(LaunchStartTask.Params.class);
        this.db = db;
        this.flowLaunchService = flowLaunchService;
        this.configurationService = configurationService;
        this.flowLaunchMutexManager = flowLaunchMutexManager;
    }

    public LaunchStartTask(LaunchId launchId) {
        super(new Params(launchId));
    }

    @Override
    public Duration getTimeout() {
        return Duration.ofMinutes(1);
    }

    @Override
    protected void execute(Params params, ExecutionContext context) {
        acquireMutexAndExecute(params.getLaunchId());
    }

    public Optional<FlowLaunchId> acquireMutexAndExecute(LaunchId launchId) {
        return flowLaunchMutexManager.acquireAndRun(launchId, () -> doExecute(launchId));
    }

    private Optional<FlowLaunchId> doExecute(LaunchId launchId) {
        log.info("Staring launch {}", launchId);

        return db.currentOrTx(() -> {
            Launch launch = db.launches().get(launchId);
            LaunchState.Status status = launch.getStatus();

            Preconditions.checkState(
                    status != LaunchState.Status.DELAYED,
                    "Cannot start launch in delayed status"
            );

            if (status != LaunchState.Status.STARTING) {
                log.info("Flow already launched: {}", launch.getState());
                return Optional.empty();
            }

            log.info("Launch: {}", launch);

            var processId = launchId.getProcessId();

            var virtualProcessId = VirtualCiProcessId.of(processId);
            var path = virtualProcessId.getResolvedPath();
            log.info("Resolved configuration to load: {} -> {}", processId, path);

            var bundle = configurationService.getConfig(path, launch.getFlowInfo().getConfigRevision());
            if (!bundle.isReadyForLaunch()) {
                throw new ConfigHasSecurityProblemsException(bundle);
            }

            bundle = bundle.withVirtualProcessId(virtualProcessId, launch.getProject());

            try {
                FlowLaunchId flowLaunchId = flowLaunchService.launchFlow(launch, bundle);
                log.info("Flow launch started {}", flowLaunchId);
                return Optional.of(flowLaunchId);
            } catch (AYamlValidationException e) {
                throw new RuntimeException(e);
            }
        });
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
