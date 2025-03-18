package ru.yandex.ci.engine.registry;

import java.util.Map;

import com.fasterxml.jackson.core.JsonProcessingException;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.config.registry.RollbackMode;
import ru.yandex.ci.core.config.registry.TaskConfigYamlParser;
import ru.yandex.ci.core.config.registry.TaskId;
import ru.yandex.ci.core.config.registry.Type;
import ru.yandex.ci.core.launch.TaskVersion;
import ru.yandex.ci.core.registry.RegistryTask;
import ru.yandex.ci.core.tasklet.TaskletMetadata;
import ru.yandex.ci.core.tasklet.TaskletRuntime;
import ru.yandex.ci.engine.EngineTestBase;
import ru.yandex.ci.test.TestUtils;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.Mockito.when;

@Import(TaskRegistrySnapshotCronTask.class)
public class TaskRegistrySnapshotCronTaskTest extends EngineTestBase {
    @Autowired
    private TaskRegistrySnapshotCronTask taskRegistrySnapshotCronTask;

    @MockBean
    private TaskRegistry taskRegistry;

    @Test
    void empty() {
        when(taskRegistry.loadRegistry()).thenReturn(Map.of());
        taskRegistrySnapshotCronTask.executeImpl(null);
        assertThat(db.currentOrReadOnly(() -> db.registryTask().countAll())).isZero();
    }

    @Test
    void updateValues() throws JsonProcessingException {

        var yaPackage = RegistryTask.builder().id(id("common/arcadia/ya_package", TaskVersion.STABLE))
                .type(Type.SANDBOX_TASK)
                .rollbackMode(RollbackMode.DENY)
                .sandboxTaskName("YA_PACKAGE")
                .build();
        var woodcutterLatest = RegistryTask.builder().id(id("common/misc/woodcutter", TaskVersion.of("latest")))
                .taskletMetadataId(woodcutter(42423523))
                .type(Type.TASKLET)
                .rollbackMode(RollbackMode.DENY)
                .build();
        var woodcutterStable = RegistryTask.builder().id(id("common/misc/woodcutter", TaskVersion.of("stable")))
                .taskletMetadataId(woodcutter(31241241))
                .type(Type.TASKLET)
                .rollbackMode(RollbackMode.DENY)
                .build();
        var woodcutterTesting = RegistryTask.builder().id(id("common/misc/woodcutter", TaskVersion.of("testing")))
                .taskletMetadataId(woodcutter(75674567))
                .type(Type.TASKLET)
                .rollbackMode(RollbackMode.DENY)
                .build();

        var taskletConfig = TaskConfigYamlParser.parse(TestUtils.textResource("task/tasklet.yaml"));
        var woodcutterTaskId = TaskId.of("common/misc/woodcutter");
        var sandboxTaskConfig = TaskConfigYamlParser.parse(TestUtils.textResource("task/sandbox-task.yaml"));
        var yaPackageTaskId = TaskId.of("common/arcadia/ya_package");


        when(taskRegistry.loadRegistry()).thenReturn(Map.of(
                woodcutterTaskId, taskletConfig,
                yaPackageTaskId, sandboxTaskConfig
        ));
        taskRegistrySnapshotCronTask.executeImpl(null);

        assertThat(db.currentOrReadOnly(() -> db.registryTask().findAlive())).containsExactlyInAnyOrder(
                yaPackage,
                woodcutterLatest,
                woodcutterStable,
                woodcutterTesting
        );


        when(taskRegistry.loadRegistry()).thenReturn(Map.of(
                yaPackageTaskId, sandboxTaskConfig
        ));
        taskRegistrySnapshotCronTask.executeImpl(null);

        assertThat(db.currentOrReadOnly(() -> db.registryTask().findAlive())).containsExactlyInAnyOrder(
                yaPackage
        );


        when(taskRegistry.loadRegistry()).thenReturn(Map.of(
                woodcutterTaskId, taskletConfig
        ));
        taskRegistrySnapshotCronTask.executeImpl(null);

        assertThat(db.currentOrReadOnly(() -> db.registryTask().findAlive())).containsExactlyInAnyOrder(
                woodcutterLatest,
                woodcutterStable,
                woodcutterTesting
        );
    }

    private TaskletMetadata.Id woodcutter(int resourceId) {
        return TaskletMetadata.Id.of("WoodcutterPy", TaskletRuntime.SANDBOX, resourceId);
    }

    private RegistryTask.Id id(String path, TaskVersion version) {
        return RegistryTask.Id.of(TaskId.of(path), version);
    }
}
