package ru.yandex.ci.engine.discovery.tier0;

import java.nio.file.Path;
import java.nio.file.Paths;
import java.time.Duration;
import java.time.temporal.ChronoUnit;
import java.util.HashSet;
import java.util.List;
import java.util.Optional;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.client.pciexpress.PciExpressClient;
import ru.yandex.ci.common.bazinga.AbstractOnetimeTask;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommitUtils;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.a.AffectedAYamlsFinder;
import ru.yandex.ci.core.config.a.ConfigChangeType;
import ru.yandex.ci.core.discovery.DiscoveryType;
import ru.yandex.ci.core.pr.RevisionNumberService;
import ru.yandex.ci.engine.config.ConfigBundle;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.engine.discovery.DiscoveryContext;
import ru.yandex.ci.engine.discovery.DiscoveryServicePostCommits;
import ru.yandex.ci.engine.launch.auto.DiscoveryProgressService;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.lang.NonNullApi;
import ru.yandex.misc.bender.annotation.BenderBindAllFields;

@Slf4j
@NonNullApi
public class ProcessPciDssCommitTask extends AbstractOnetimeTask<ProcessPciDssCommitTask.Parameters> {

    private PciExpressClient pciExpressClient;
    private RevisionNumberService revisionNumberService;
    private ArcService arcService;
    private ConfigurationService configurationService;
    private DiscoveryServicePostCommits discoveryServicePostCommits;
    private DiscoveryProgressService discoveryProgressService;

    public ProcessPciDssCommitTask(PciExpressClient pciExpressClient,
                                   RevisionNumberService revisionNumberService,
                                   ArcService arcService,
                                   ConfigurationService configurationService,
                                   DiscoveryServicePostCommits discoveryServicePostCommits,
                                   DiscoveryProgressService discoveryProgressService) {
        super(Parameters.class);
        this.pciExpressClient = pciExpressClient;
        this.revisionNumberService = revisionNumberService;
        this.arcService = arcService;
        this.configurationService = configurationService;
        this.discoveryServicePostCommits = discoveryServicePostCommits;
        this.discoveryProgressService = discoveryProgressService;
    }

    public ProcessPciDssCommitTask(Parameters params) {
        super(params);
    }

    @Override
    protected void execute(Parameters params, ExecutionContext context) throws Exception {
        var paths = pciExpressClient.getPathsByCommitId(params.getCommitId());

        var targetAYamls = new HashSet<Path>();
        for (var path : paths) {
            AffectedAYamlsFinder.addPotentialConfigsRecursively(Paths.get(path), false, targetAYamls);
        }

        for (var aYaml : targetAYamls) {
            var configBundle = getConfigBundle(params.getCommitId(), aYaml).orElse(null);
            if (configBundle == null) {
                continue;
            }

            if (!configBundle.getStatus().isValidCiConfig()) {
                log.info("Config {} not in valid state ({}), skipping.", aYaml, configBundle.getStatus());
                continue;
            }

            var ciConfig = configBundle.getValidAYamlConfig().getCi();
            var discoveryContext = createContext(configBundle, paths);

            var triggeredProcesses = discoveryServicePostCommits.findTriggeredProcesses(discoveryContext, ciConfig);
            discoveryServicePostCommits.processTriggeredProcesses(discoveryContext, triggeredProcesses);
        }

        discoveryProgressService.markAsDiscovered(getOrderedArcRevision(params.getCommitId()), DiscoveryType.PCI_DSS);
    }

    @Override
    public Duration getTimeout() {
        return Duration.of(10, ChronoUnit.MINUTES);
    }

    private Optional<ConfigBundle> getConfigBundle(String commitId, Path aYamlPath) {
        var orderedArcRevision = getOrderedArcRevision(commitId);
        var commit = arcService.getCommit(orderedArcRevision);

        return configurationService
                .getOrCreateBranchConfig(aYamlPath, commit, orderedArcRevision)
                .map(configurationService::getLastActualConfig);
    }

    private DiscoveryContext createContext(ConfigBundle configBundle, List<String> affectedPaths) {
        var revision = configBundle.getConfigEntity().getRevision();
        var commit = arcService.getCommit(revision);
        var previousRevision = ArcCommitUtils.firstParentArcRevision(commit)
                .orElseThrow(() -> new IllegalArgumentException(
                        "commit %s should have at least one parent".formatted(revision)
                ));

        return DiscoveryContext.builder()
                .revision(revision)
                .previousRevision(previousRevision)
                .commit(commit)
                .configChange(ConfigChangeType.NONE)
                .configBundle(configBundle)
                .discoveryType(DiscoveryType.PCI_DSS)
                .affectedPaths(affectedPaths)
                .build();
    }

    private OrderedArcRevision getOrderedArcRevision(String commitId) {
        return revisionNumberService.getOrderedArcRevision(ArcBranch.trunk(), ArcRevision.of(commitId));
    }

    @BenderBindAllFields
    public static final class Parameters {

        private final String commitId;

        public Parameters(String commitId) {
            this.commitId = commitId;
        }

        public String getCommitId() {
            return commitId;
        }
    }

}
