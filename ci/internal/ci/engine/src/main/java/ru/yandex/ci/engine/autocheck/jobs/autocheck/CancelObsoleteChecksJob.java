package ru.yandex.ci.engine.autocheck.jobs.autocheck;

import java.util.UUID;

import javax.annotation.Nonnull;

import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.engine.autocheck.AutocheckService;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.ExecutorInfo;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;

@Slf4j
@RequiredArgsConstructor
@ExecutorInfo(
        title = "Cancel obsolete checks",
        description = "Internal job for cancelling obsolete checks"
)
public class CancelObsoleteChecksJob implements JobExecutor {

    public static final UUID ID = UUID.fromString("59405ed6-a21e-4295-a94f-507318bb6ed6");

    @Nonnull
    private final AutocheckService autocheckService;

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }

    @Override
    public void execute(JobContext context) throws Exception {
        log.info("Starting obsolete checks cancellation");
        autocheckService.cancelAutocheckPrecommitFlow(
                context.getFlowLaunch().getIdString(),
                context.getFlowLaunch().getVcsInfo()
        );
        log.info("Finished obsolete checks cancellation");
    }
}
