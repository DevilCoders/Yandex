package ru.yandex.ci.engine.autocheck.jobs.autocheck;

import java.util.Objects;
import java.util.UUID;
import java.util.stream.Collectors;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.autocheck.Autocheck;
import ru.yandex.ci.client.storage.StorageApiClient;
import ru.yandex.ci.core.resources.Resource;
import ru.yandex.ci.engine.autocheck.AutocheckService;
import ru.yandex.ci.engine.autocheck.model.CheckRecheckLaunchParams;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.ExecutorInfo;
import ru.yandex.ci.flow.engine.definition.resources.Produces;
import ru.yandex.ci.flow.utils.UrlService;
import ru.yandex.ci.storage.api.StorageApi;
import ru.yandex.ci.storage.core.CheckIteration;

@ExecutorInfo(
        title = "Start autocheck recheck",
        description = "Internal job for starting autocheck recheck task"
)
@Produces(single = Autocheck.AutocheckRecheckLaunch.class)
@Slf4j
public class StartAutocheckRecheckJob extends StartAutocheckBaseJob {
    public static final UUID ID = UUID.fromString("053dfa83-d636-45e2-bda3-3c459f2900f4");

    private final AutocheckService autocheckService;
    private final StorageApiClient storageApiClient;

    public StartAutocheckRecheckJob(
            AutocheckService autocheckService,
            StorageApiClient storageApiClient,
            UrlService urlService
    ) {
        super(urlService);
        this.autocheckService = autocheckService;
        this.storageApiClient = storageApiClient;
    }

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }

    @Override
    public void execute(JobContext context) throws Exception {
        var flowVars = Objects.requireNonNull(context.getFlowLaunch().getFlowInfo().getFlowVars()).getData();
        var checkId = flowVars.get("ci_check_id").getAsLong();
        var iterationType = CheckIteration.IterationType.forNumber(flowVars.get("ci_iteration_type").getAsInt());
        var iterationNumber = flowVars.get("ci_iteration_number").getAsInt();

        log.info("Starting for {}/{}/{}", checkId, iterationType, iterationNumber);

        var check = storageApiClient.getCheck(checkId);

        var iteration = storageApiClient.getIteration(
                CheckIteration.IterationId.newBuilder()
                        .setCheckId(Long.toString(checkId))
                        .setCheckType(iterationType)
                        .setNumber(iterationNumber)
                        .build()
        );

        var suites = storageApiClient.getSuiteRestarts(
                CheckIteration.IterationId.newBuilder()
                        .setCheckId(Long.toString(checkId))
                        .setCheckType(iterationType)
                        .setNumber(iterationNumber)
                        .build()
        );

        log.info("Suites restarts: {}", suites.size());

        log.info(
                suites.stream()
                        .map(this::formatSuite)
                        .collect(Collectors.joining("\n"))
        );

        var launch = autocheckService.createAutocheckLaunch(
                CheckRecheckLaunchParams.builder()
                        .check(check)
                        .iteration(iteration)
                        .gsidBase(generateGsidBase(context))
                        .suites(suites)
                        .build()
        );

        updateTaskBadge(context, launch.getCheckInfo().getCheckId());

        context.resources().produce(Resource.of(launch, "launch"));
    }

    private String formatSuite(StorageApi.SuiteRestart suite) {
        return "%d/%s/%d/%s".formatted(
                suite.getSuiteId(), suite.getJobName(), suite.getPartition(), suite.getIsRight() ? "right" : "left"
        );
    }
}
