package ru.yandex.ci.engine.registry;

import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Map;
import java.util.Optional;

import com.fasterxml.jackson.core.JsonProcessingException;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.Mock;
import org.mockito.junit.jupiter.MockitoExtension;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.config.registry.TaskConfig;
import ru.yandex.ci.core.config.registry.TaskId;
import ru.yandex.ci.core.config.registry.TaskRegistryException;
import ru.yandex.ci.core.test.TestData;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;
import static org.mockito.Mockito.when;
import static ru.yandex.ci.test.TestUtils.textResource;

@ExtendWith(MockitoExtension.class)
class TaskRegistryImplTest {
    private TaskRegistry taskRegistry;

    @Mock
    private ArcService arcService;

    @BeforeEach
    public void setUp() {
        taskRegistry = new TaskRegistryImpl(arcService, null);
    }

    @Test
    public void shouldGetContent() throws JsonProcessingException {
        var commit = TestData.REVISION_COMMIT;
        var revision = commit.getRevision();
        when(arcService.getCommit(revision)).thenReturn(commit);
        when(arcService.getFileContent(Path.of("ci/registry/woodflow/woodcutter.yaml"), revision))
                .thenReturn(Optional.of(textResource("task/tasklet.yaml")));

        TaskConfig task = taskRegistry.lookup(revision, TaskId.of("woodflow/woodcutter"));

        assertThat(task).isNotNull();
        assertThat(task.getTasklet()).isNotNull();
        assertThat(task.getTasklet().getImplementation()).isEqualTo("WoodcutterPy");
    }

    @Test
    public void throwExceptionIfNotFound() {
        assertThatThrownBy(() ->
                taskRegistry.lookup(ArcRevision.of("DEADBEEF"), TaskId.of("woodflow/woodcutter"))
        ).hasMessage("yaml for woodflow/woodcutter expected" +
                " at ci/registry/woodflow/woodcutter.yaml" +
                " in revision DEADBEEF not found"
        ).isInstanceOf(TaskRegistryException.class);
    }

    @Test
    void testPath() {
        assertThat(TaskRegistryImpl.getVcsPath(TaskId.of("demo/woodflow/woodcutter")))
                .isEqualTo(Path.of("ci/registry/demo/woodflow/woodcutter.yaml"));

        assertThat(TaskRegistryImpl.taskIdFromPath(Path.of("ci/registry/demo/woodflow/woodcutter.yaml")))
                .isEqualTo(TaskId.of("demo/woodflow/woodcutter"));

        assertThat(TaskRegistryImpl.taskIdFromPath(Path.of("ci/registry/woodcutter.yaml")))
                .isEqualTo(TaskId.of("woodcutter"));
    }

    @Test
    void loadRegistry() {
        var revision = TestData.REVISION;
        var commit = TestData.REVISION_COMMIT;
        var taskConfigs = Map.of(
                Path.of("common/test/tasklet.yaml"), textResource("task/tasklet.yaml"),
                Path.of("common/test/sandbox.yaml"), textResource("task/sandbox-task.yaml")
        );
        var filesInRegistryFolder = new ArrayList<>(taskConfigs.keySet());
        filesInRegistryFolder.add(Path.of("a.yaml"));
        filesInRegistryFolder.add(Path.of("common/test/README.md"));
        filesInRegistryFolder.add(Path.of("common/test/invalid/invalid-config.yaml"));

        when(arcService.getLastRevisionInBranch(ArcBranch.trunk())).thenReturn(revision);
        when(arcService.getCommit(revision)).thenReturn(commit);
        when(arcService.listDir("ci/registry", revision, true, true)).thenReturn(filesInRegistryFolder);

        var registryRoot = Path.of("ci/registry");
        for (var entry : taskConfigs.entrySet()) {
            when(arcService.getFileContent(registryRoot.resolve(entry.getKey()), revision))
                    .thenReturn(Optional.of(entry.getValue()));
        }
        when(arcService.getFileContent(
                Path.of("ci/registry/common/test/invalid/invalid-config.yaml"), revision)
        ).thenReturn(Optional.of("""
                valid-yaml: true
                valid-task-config: false
                """));

        var registry = taskRegistry.loadRegistry();
        assertThat(registry).containsOnlyKeys(TaskId.of("common/test/tasklet"), TaskId.of("common/test/sandbox"));
        assertThat(registry.values()).extracting(TaskConfig::getTitle)
                .containsExactlyInAnyOrder("Имя sandbox-задачи", "Имя тасклета");
    }
}
