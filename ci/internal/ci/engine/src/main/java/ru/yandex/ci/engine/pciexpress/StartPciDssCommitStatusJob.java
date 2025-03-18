package ru.yandex.ci.engine.pciexpress;

import java.nio.file.Path;
import java.util.UUID;

import com.google.protobuf.TextFormat;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.resources.Resource;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.ExecutorInfo;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.definition.resources.Produces;
import ru.yandex.repo.pciexpress.proto.Configuration;
import ru.yandex.repo.pciexpress.proto.Dto;

@Slf4j
@ExecutorInfo(
        title = "PCI-DSS commit status job",
        description = "Create build graph for configured projects and check if commit files in."
)
@Produces(single = Pciexpress.PciExpressTaskData.class)
@RequiredArgsConstructor
public class StartPciDssCommitStatusJob implements JobExecutor {

    public static final UUID ID = UUID.fromString("350f10e3-99a0-4248-a44c-2ced9a0c8725");
    private final ArcService arcService;
    private final String arcEendpoint;
    static final Path PCI_DSS_CONFIGURATION = Path.of("repo/pciexpress/configuration.pb");

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }

    private String getArcApiUrl() {
        var host = arcEendpoint.split(":", 2)[0];
        return host;
    }
    protected static String getTaskConfiguration(String configurationContent, String commitId) throws Exception {
        var builder = Configuration.TPciExpressConfiguration.newBuilder();
        TextFormat.getParser().merge(configurationContent, builder);
        var configuration = builder.build();
        var legsBuilder = Dto.LegsInput.newBuilder();
        legsBuilder.setCommitId(commitId);
        configuration.getProjectsList().forEach(project -> {
            project.getPackagesList().forEach(pkg -> {
                var pkgInfoBuilder = Dto.PackageInfo.newBuilder();
                pkgInfoBuilder.setPackage(pkg);
                legsBuilder.addPackages(pkgInfoBuilder);
            });
        });
        var units = legsBuilder.build();
        log.info("Commit packages count: {}", units.getPackagesCount());
        return units.toString();
    }

    @Override
    public void execute(JobContext context) throws Exception {
        log.info("Starting PciExpress job...");
        var vcsInfo = context.getFlowLaunch().getVcsInfo();
        var revision = vcsInfo.getRevision();
        log.info("Revision: {}", revision);
        var proto = arcService.getFileContent(PCI_DSS_CONFIGURATION, revision);
        if (proto.isPresent()) {
            var output = Pciexpress.PciExpressTaskData.newBuilder();
            output.setUnits(getTaskConfiguration(proto.get(), revision.getCommitId()));
            var arcApiUrl = getArcApiUrl();
            log.info("arcApiUrl: {}", arcApiUrl);
            output.setArcServerUrl(arcApiUrl);
            context.resources().produce(Resource.of(output.build(), "pciexpress"));
        }

    }
}
