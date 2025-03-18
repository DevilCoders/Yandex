package ru.yandex.ci.core.tasklet;

import java.util.HashMap;
import java.util.NoSuchElementException;
import java.util.Objects;
import java.util.Optional;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.Mockito;
import org.springframework.boot.test.mock.mockito.MockBean;

import ru.yandex.ci.CommonTestBase;
import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.client.sandbox.api.ResourceInfo;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.db.TestCiDbUtils;
import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.test.TestUtils;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static ru.yandex.ci.test.TestUtils.parseJson;

class TaskletMetadataServiceImplTest extends CommonTestBase {
    private static final long SANDBOX_RESOURCE_ID = 747;

    private TaskletMetadataService metadataService;

    @MockBean
    private SandboxClient sandboxClient;
    @MockBean
    private SchemaService schemaService;
    @MockBean
    private CiMainDb db;
    private static final TaskletMetadata.Id SAWMILL_ID = TaskletMetadata.Id.of("SawmillPy", TaskletRuntime.SANDBOX,
            SANDBOX_RESOURCE_ID);

    @BeforeEach
    public void setUp() {
        TestCiDbUtils.mockToCallRealTxMethods(db);

        var table = Mockito.mock(TaskletMetadataTable.class);

        when(db.taskletMetadata()).thenReturn(table);
        var store = new HashMap<TaskletMetadata.Id, TaskletMetadata>();
        //noinspection SuspiciousMethodCalls
        when(table.find(any(TaskletMetadata.Id.class))).then(invocation ->
                Optional.ofNullable(store.get(invocation.getArgument(0))));
        //noinspection SuspiciousMethodCalls
        when(table.get(any(TaskletMetadata.Id.class))).then(invocation ->
                Objects.requireNonNullElseGet(store.get(invocation.getArgument(0)), NoSuchElementException::new));
        when(table.save(any(TaskletMetadata.class))).then(invocation -> {
            var metadata = (TaskletMetadata) invocation.getArgument(0);
            store.put(metadata.getId(), metadata);
            return metadata;
        });

        metadataService = new TaskletMetadataServiceImpl(sandboxClient, this.db, schemaService, null);
    }

    @Test
    public void fetchOk() {
        when(sandboxClient.getResourceInfo(SANDBOX_RESOURCE_ID))
                .thenReturn(parseJson("tasklet-schema/sandbox-attributes.json", ResourceInfo.class));

        TaskletMetadata metadata = metadataService.fetchMetadata(SAWMILL_ID);

        TaskletMetadata inRepository = db.currentOrReadOnly(() -> db.taskletMetadata().get(SAWMILL_ID));

        var loadedId = metadata.getId();
        assertThat(loadedId.getRuntime()).isEqualTo(TaskletRuntime.SANDBOX);
        assertThat(loadedId.getSandboxResourceId()).isEqualTo(SANDBOX_RESOURCE_ID);
        assertThat(metadata.getName()).isEqualTo("Sawmill");
        assertThat(metadata.getSandboxTask()).isEqualTo("TASKLET_SAWMILL");
        assertThat(loadedId.getImplementation()).isEqualTo("SawmillPy");
        assertThat(metadata.getInputType()).isEqualTo(JobResourceType.of("WoodflowCi.Input"));
        assertThat(metadata.getOutputType()).isEqualTo(JobResourceType.of("WoodflowCi.Output"));
        assertThat(metadata.getFeatures()).isEqualTo(Features.empty());
        assertThat(metadata).isEqualTo(inRepository);
    }

    @Test
    public void fetchWithFeatures() {
        when(sandboxClient.getResourceInfo(SANDBOX_RESOURCE_ID))
                .thenReturn(parseJson("tasklet-schema/sandbox-attributes-with-features.json", ResourceInfo.class));

        TaskletMetadata metadata = metadataService.fetchMetadata(SAWMILL_ID);

        TaskletMetadata inRepository = db.currentOrReadOnly(() -> db.taskletMetadata().get(SAWMILL_ID));

        var expected = TaskletMetadata.builder()
                .id(TaskletMetadata.Id.of("SawmillPy", TaskletRuntime.SANDBOX, SANDBOX_RESOURCE_ID))
                .name("Sawmill")
                .sandboxTask("TASKLET_SAWMILL")
                .inputType(JobResourceType.of("WoodflowCi.Input"))
                .outputType(JobResourceType.of("WoodflowCi.Output"))
                .descriptors(TestUtils.binaryResource("tasklet-schema/sawmill-descriptors.bin"))
                .features(new Features(true))
                .build();

        assertThat(metadata).isEqualTo(expected);

        assertThat(metadata).isEqualTo(inRepository);
    }

    @Test
    public void fetchInvalidArch() {
        var resource = parseJson("tasklet-schema/sandbox-attributes.json", ResourceInfo.class).toBuilder()
                .arch("any")
                .build();
        when(sandboxClient.getResourceInfo(SANDBOX_RESOURCE_ID))
                .thenReturn(resource);

        assertThatThrownBy(() -> metadataService.fetchMetadata(SAWMILL_ID))
                .isInstanceOf(TaskletMetadataValidationException.class)
                .hasMessage("resource 747 is compiled for arch any. All Tasklets V1 must be compiled for Linux");
    }

    @Test
    public void shouldNoFetchIfPresentInRepository() {
        TaskletMetadata metadata = new TaskletMetadata.Builder()
                .id(TaskletMetadata.Id.of("SawmillPy", TaskletRuntime.SANDBOX, SANDBOX_RESOURCE_ID))
                .build();

        db.currentOrTx(() ->
                db.taskletMetadata().save(metadata));

        TaskletMetadata fetched = metadataService.fetchMetadata(metadata.getId());

        assertThat(fetched).isEqualTo(metadata);
        verify(sandboxClient, never()).getResourceInfo(SANDBOX_RESOURCE_ID);
    }
}
