package ru.yandex.ci.engine.pciexpress;

import java.util.UUID;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.client.sandbox.ProxySandboxClient;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.ExecutorInfo;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.definition.resources.Consume;
import ru.yandex.ci.flow.engine.definition.resources.Produces;
import ru.yandex.ci.tasklet.SandboxResource;
import ru.yandex.repo.pciexpress.proto.Dto;

@Slf4j
@ExecutorInfo(
        title = "Get PCI-DSS commit info",
        description = "Process commit graph results"
)
@Produces(single = Pciexpress.PciExpressTaskData.class)
@Consume(name = "sb_resources", proto = SandboxResource.class, list = true)
public class GetPciDssCommitResultJob implements JobExecutor {
    public static final UUID ID = UUID.fromString("55d8640c-f943-11ec-a29b-525400123456");

    protected ProxySandboxClient proxySandboxClient;

    public GetPciDssCommitResultJob(ProxySandboxClient proxySandboxClient) {
        this.proxySandboxClient = proxySandboxClient;
    }

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }

    public static void processTaskResult(Dto.LegsOutput result) {
        result.getResultsList().forEach((var buildResult) -> {
            if (buildResult.hasError()) {
                log.info("buildResult.hasError = true");
            }
            if (buildResult.hasPackage()) {
                var pkg = buildResult.getPackage();
                log.info("pkg.getPackage() = {}", pkg.getPackage());
                var files = pkg.getFilesList();
                log.info("pkg.getFilesList() = {}", files);
            }
        });
    }

    @Override
    public void execute(JobContext context) throws Exception {
        var resources = context.resources().consumeList(SandboxResource.class);
        var res = resources.stream().filter(item -> item.getType().equals("PCI_EXPRESS_JOG")).findFirst();
        var isPresent = res.isPresent();
        log.info("resource.isPresent = {}", isPresent);
        if (isPresent) {
            var resourceId = res.get().getId();
            log.info("resource: {}", resourceId);
            var resourceData = proxySandboxClient.downloadResource(resourceId);
            var output = Dto.LegsOutput.parseFrom(resourceData.getStream());
            log.info("commit: {}", output.getCommitId());
            processTaskResult(output);
        }
    }
}

