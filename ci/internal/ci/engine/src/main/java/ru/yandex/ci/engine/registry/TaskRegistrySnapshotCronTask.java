package ru.yandex.ci.engine.registry;

import java.time.Duration;
import java.util.Set;

import javax.annotation.Nullable;

import lombok.extern.slf4j.Slf4j;
import one.util.streamex.EntryStream;
import org.apache.curator.framework.CuratorFramework;

import ru.yandex.ci.core.config.registry.Type;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.launch.TaskVersion;
import ru.yandex.ci.engine.common.CiEngineCronTask;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;

@Slf4j
public class TaskRegistrySnapshotCronTask extends CiEngineCronTask {

    private final CiMainDb db;
    private final TaskRegistry taskRegistry;

    public TaskRegistrySnapshotCronTask(CiMainDb db, TaskRegistry taskRegistry, @Nullable CuratorFramework curator) {
        super(Duration.ofHours(2), Duration.ofMinutes(10), curator);
        this.db = db;
        this.taskRegistry = taskRegistry;
    }

    @Override
    public final void executeImpl(ExecutionContext executionContext) {
        log.info("Making registry snapshot");
        db.currentOrTx(() -> {
            var registry = taskRegistry.loadRegistry();
            var registryTasks = EntryStream.of(registry)
                    .filterKeyValue((taskId, config) -> {
                        var type = Type.of(config, taskId);
                        return type == Type.TASKLET || type == Type.TASKLET_V2 || type == Type.SANDBOX_TASK;
                    })
                    .flatMapKeyValue(
                            (taskId, config) -> {
                                var taskVersions = config.getVersions().keySet();
                                if (taskVersions.isEmpty()) {
                                    taskVersions = Set.of(TaskVersion.STABLE);
                                }
                                return taskVersions.stream()
                                        .map(version -> TaskRegistryImpl.from(taskId, version, config));
                            }
                    )
                    .toList();

            db.registryTask().markStaleAll();
            db.registryTask().save(registryTasks);
            log.info("Registry tasks: {}", registryTasks.size());
        });
        log.info("Done.");
    }

}
