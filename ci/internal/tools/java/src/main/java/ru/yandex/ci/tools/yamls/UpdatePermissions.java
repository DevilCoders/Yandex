package ru.yandex.ci.tools.yamls;

import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.github.fge.jsonschema.core.exceptions.ProcessingException;
import com.google.common.base.Preconditions;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.config.ConfigEntity;
import ru.yandex.ci.core.config.ConfigPermissions;
import ru.yandex.ci.engine.config.ConfigParseService;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.engine.spring.ConfigurationServiceConfig;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Configuration
@Import(ConfigurationServiceConfig.class)
public class UpdatePermissions extends AbstractSpringBasedApp {

    @Autowired
    private CiDb db;

    @Autowired
    private ConfigParseService configParseService;

    @Autowired
    private ConfigurationService configurationService;

    private final AtomicInteger scheduled = new AtomicInteger();
    private final AtomicInteger parsed = new AtomicInteger();

    @SuppressWarnings({"ResultOfMethodCallIgnored", "FutureReturnValueIgnored"})
    @Override
    protected void run() throws JsonProcessingException, ProcessingException, ExecutionException, InterruptedException {

        var parseExecutor = Executors.newFixedThreadPool(32);
        var updateExecutor = Executors.newFixedThreadPool(32);

        db.currentOrReadOnly(() -> db.configHistory().streamAll(1000)).forEach(configEntity -> {
            if (!acceptConfig(configEntity)) {
                return;
            }
            scheduled.incrementAndGet();
            if (scheduled.get() % 1000 == 0) {
                log.info("Checked {} configs...", scheduled.get());
            }
            parseExecutor.submit(() -> parseConfig(configEntity, updateExecutor));
        });
        log.info("Loaded {} configs...", scheduled.get());

        parseExecutor.shutdown();
        parseExecutor.awaitTermination(30, TimeUnit.MINUTES);

        updateExecutor.shutdown();
        updateExecutor.awaitTermination(30, TimeUnit.MINUTES);

        log.info("Total configs processed: {} out {}", parsed.get(), scheduled.get());
    }

    private boolean acceptConfig(ConfigEntity configEntity) {
        return configEntity.getPermissions() == null && configEntity.getStatus().isValidCiConfig();
    }

    @SuppressWarnings("FutureReturnValueIgnored")
    private void parseConfig(ConfigEntity configEntity, ExecutorService executor) {
        ConfigUpdate update;
        try {
            var config = configParseService.parseAndValidate(
                    configEntity.getConfigPath(),
                    configEntity.getRevision().toRevision(),
                    configEntity.getTaskRevisions()
            );
            var permissions = configurationService.createConfigPermissions(config, configEntity.getStatus());
            Preconditions.checkState(permissions != null,
                    "Internal error. Unable to calculate permissions for %s", configEntity.getId());
            update = new ConfigUpdate(configEntity.getId(), permissions);
        } catch (Exception e) {
            log.error("Unable parse config {} at {}", configEntity.getConfigPath(), configEntity.getRevision(), e);
            return; // ---
        }
        executor.submit(() -> updateConfig(update));
    }

    private void updateConfig(ConfigUpdate update) {
        try {
            log.info("[{}/{}] {} -> {}", parsed.incrementAndGet(), scheduled.get(), update.id, update.permissions);
            db.currentOrTx(() -> {
                var config = db.configHistory().get(update.id);
                if (acceptConfig(config)) {
                    db.configHistory().save(config.withPermissions(update.permissions));
                }
            });
        } catch (Exception e) {
            log.error("Unable to update config {}", update, e);
        }
    }

    @Value
    private static class ConfigUpdate {
        @Nonnull
        ConfigEntity.Id id;
        @Nullable
        ConfigPermissions permissions;
    }

    public static void main(String[] args) {
        startAndStopThisClass(args);
    }
}
