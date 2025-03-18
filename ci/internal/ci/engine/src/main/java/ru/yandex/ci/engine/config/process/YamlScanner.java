package ru.yandex.ci.engine.config.process;

import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.function.Supplier;
import java.util.stream.Collectors;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.JsonNode;
import com.google.common.base.Suppliers;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.config.a.AYamlParser;
import ru.yandex.ci.core.db.model.ConfigState;
import ru.yandex.ci.engine.config.ConfigParseService;
import ru.yandex.ci.flow.db.CiDb;

@RequiredArgsConstructor
@Slf4j
public abstract class YamlScanner {
    private final CiDb db;
    private final ConfigParseService configParseService;
    private final ArcService arcService;

    private final Supplier<ArcRevision> workingRevisionSupplier =
            Suppliers.memoize(this::calculateWorkingRevision);

    protected boolean acceptConfig(ConfigState cfg) {
        return true;
    }

    protected ArcRevision calculateWorkingRevision() {
        var trunk = ArcBranch.trunk();
        var lastRevision = arcService.getLastRevisionInBranch(trunk);
        log.info("Last revision in {}: {}", trunk, lastRevision);
        return lastRevision;
    }

    public void checkAllConfigs(ParsedPathListener parsedPathListener) throws Exception {
        processImpl((path, commitId) -> {
            try {
                var result = configParseService.parseAndValidate(path, commitId, null);
                parsedPathListener.onParsedConfig(path, result);
            } catch (Exception e) {
                log.warn("Unable to load file {}: {}", path, e.getMessage());
            }
        });
    }

    public void checkAllConfigs(PathListener pathListener) throws Exception {
        processImpl((path, commitId) -> pathListener.onConfig(path));
    }

    private void processImpl(PathReferenceListener pathListener) throws Exception {
        var workingRevision = getWorkingRevision();
        var commitId = workingRevision.getCommitId();

        List<ConfigState> configs = db.currentOrReadOnly(() ->
                db.configStates().streamAll(1000)
                        .filter(cfg -> {
                            log.info("[{}] {}", cfg.getStatus(), cfg.getConfigPath());
                            return !cfg.getStatus().isHidden() && acceptConfig(cfg);
                        })
                        .collect(Collectors.toList()));

        log.info("Checking {} file(s)...", configs.size());

        ExecutorService executor = Executors.newFixedThreadPool(16);

        List<Future<?>> futures = new ArrayList<>();
        for (var config : configs) {
            futures.add(executor.submit(() -> {
                try {
                    pathListener.onConfig(config.getConfigPath(), ArcRevision.of(commitId));
                } catch (Exception e) {
                    throw new RuntimeException(e);
                }
            }));
        }
        for (var future : futures) {
            try {
                future.get();
            } catch (Exception e) {
                log.error("Error when processing path", e);
            }
        }
        executor.shutdown();
        if (!executor.awaitTermination(30, TimeUnit.MINUTES)) {
            log.error("Unable to wait for parser completion");
        }
    }

    protected final ArcRevision getWorkingRevision() {
        return workingRevisionSupplier.get();
    }

    public Optional<String> getContent(Path configPath) {
        return getContent(configPath, getWorkingRevision());
    }

    public Optional<String> getContent(Path configPath, ArcRevision revision) {
        return arcService.getFileContent(configPath, revision);
    }

    public JsonNode getYamlTree(String content) throws JsonProcessingException {
        return AYamlParser.getMapper().readTree(content);
    }
}
