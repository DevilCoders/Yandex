package ru.yandex.ci.engine.config.process;

import java.time.Duration;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import org.apache.curator.framework.CuratorFramework;

import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.a.model.ReleaseConfig;
import ru.yandex.ci.core.db.model.ConfigState;
import ru.yandex.ci.engine.common.CiEngineCronTask;
import ru.yandex.ci.engine.config.ConfigParseService;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;

public class CheckMinCommitsZeroCronTask extends CiEngineCronTask {

    private final CiDb db;
    private final ConfigParseService configParseService;
    private final ArcService arcService;

    public CheckMinCommitsZeroCronTask(
            CiDb db,
            ConfigParseService configParseService,
            ArcService arcService,
            @Nullable CuratorFramework curator
    ) {
        super(Duration.ofHours(6), Duration.ofMinutes(10), curator);
        this.db = db;
        this.configParseService = configParseService;
        this.arcService = arcService;
    }

    @Override
    protected void executeImpl(ExecutionContext executionContext) throws Exception {
        var scanner = new YamlScanner(db, configParseService, arcService) {
            @Override
            protected boolean acceptConfig(ConfigState cfg) {
                return cfg.getStatus() == ConfigState.Status.OK
                        &&
                        cfg.getReleases().stream()
                                .anyMatch(r -> r.getAuto().isEnabled() || r.getBranchesAuto().isEnabled());
            }
        };

        var processesWithMinCommits0 = new ArrayList<CiProcessId>();

        scanner.checkAllConfigs((path, result) ->
                Objects.requireNonNull(result.getAYamlConfig()).getCi()
                        .getReleases().entrySet()
                        .stream()
                        .filter(this::hasAutoWithMinCommits0)
                        .map(e -> CiProcessId.ofRelease(path, e.getKey()))
                        .forEach(processesWithMinCommits0::add));

        log.info("Found {} processes with min-commits: 0:\n{}",
                processesWithMinCommits0.size(),
                separateItemsPerLines(processesWithMinCommits0)
        );

        var notInQueueProcesses = getCiProcessesNotInQueue(processesWithMinCommits0);

        log.info("Found {} processes with min-commits: 0:\n{}",
                notInQueueProcesses.size(),
                separateItemsPerLines(notInQueueProcesses)
        );

        if (!notInQueueProcesses.isEmpty()) {
            throw new IllegalStateException("found processes with min-commits: 0 but without queue item: " +
                    separateItemsPerLines(notInQueueProcesses));
        }
    }

    private String separateItemsPerLines(Collection<?> items) {
        return items.stream().map(Object::toString).collect(Collectors.joining("\n"));
    }

    private Set<CiProcessId> getCiProcessesNotInQueue(Collection<CiProcessId> processes) {
        return db.currentOrReadOnly(() -> {
            var result = new HashSet<CiProcessId>();
            for (var process : processes) {
                var queue = db.autoReleaseQueue().findByProcessId(process);
                if (queue.isEmpty()) {
                    result.add(process);
                }
            }
            return result;
        });
    }

    private boolean hasAutoWithMinCommits0(Map.Entry<String, ReleaseConfig> e) {
        var auto = e.getValue().getAuto();
        var zero = Integer.valueOf(0);
        return auto != null && auto.getConditions().stream().anyMatch(c -> zero.equals(c.getMinCommits()));
    }
}
