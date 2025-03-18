package ru.yandex.ci.tools.yamls;

import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.function.Predicate;

import javax.annotation.Nonnull;

import lombok.Value;
import lombok.extern.slf4j.Slf4j;
import org.springframework.context.annotation.Configuration;

import ru.yandex.ci.core.config.ConfigProblem;
import ru.yandex.ci.engine.config.ConfigParseResult;
import ru.yandex.ci.tools.ProcessAYamlsBase;

@Slf4j
@Configuration
public class ValidateAllAYamls extends ProcessAYamlsBase {

    @Override
    protected void run() throws Exception {
        var valid = new AtomicInteger();
        var invalid = new AtomicInteger();
        var notCi = new AtomicInteger();
        var invalidCfg = Collections.synchronizedList(new ArrayList<YamlResult>());

        var scanner = new YamlScanner();
        scanner.checkAllConfigs((path, result) -> {
            var status = result.getStatus();
            log.info("Configuration {} is {}", path, status);
            switch (status) {
                case VALID -> valid.incrementAndGet();
                case INVALID -> {
                    invalid.incrementAndGet();
                    invalidCfg.add(new YamlResult(path, result));
                }
                case NOT_CI -> notCi.incrementAndGet();
                default -> throw new IllegalStateException("Unsupported status: " + status);
            }
        });

        log.info("Total valid: {}", valid);
        log.info("Total invalid: {}", invalid);
        log.info("Total not CI: {}", notCi);

        List<YamlResult> invalidAYamls = new ArrayList<>();
        Predicate<ConfigProblem> invalidAYamlFilter = problem ->
                problem.getTitle().contains("Inconsistent type in json schema")
                        || problem.getDescription().contains("Inconsistent type in json schema");

        log.info("Total {} invalid config(s):", invalidCfg.size());
        for (var cfg : invalidCfg) {
            log.info("{} is {}", cfg.getPath(), cfg.getResult().getStatus());
            for (var problem : cfg.getResult().getProblems()) {
                log.info("+ Problem: {}", problem);
            }
            for (var problem : cfg.getResult().getProblems()) {
                if (invalidAYamlFilter.test(problem)) {
                    invalidAYamls.add(cfg);
                    break;
                }
            }
        }

        log.info("{} specific invalid configs(s)", invalidAYamls.size());
        invalidAYamls.forEach(cfg -> log.info("Invalid {}: {}", cfg.getPath(), cfg.getResult().getProblems()));
    }

    public static void main(String[] args) {
        startAndStopThisClass(args, Environment.STABLE);
    }

    @Value
    static class YamlResult {
        @Nonnull
        Path path;
        @Nonnull
        ConfigParseResult result;
    }
}
