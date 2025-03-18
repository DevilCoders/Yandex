package ru.yandex.ci.engine.launch;

import java.nio.file.Path;
import java.util.Map;
import java.util.Set;

import com.google.gson.JsonParser;
import lombok.extern.slf4j.Slf4j;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.junit.jupiter.MockitoExtension;

import ru.yandex.ci.CommonTestBase;
import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.core.config.a.AYamlParser;
import ru.yandex.ci.core.config.a.model.AYamlConfig;
import ru.yandex.ci.core.config.a.validation.AYamlValidationException;
import ru.yandex.ci.core.config.a.validation.ValidationReport;
import ru.yandex.ci.core.config.registry.TaskConfig;
import ru.yandex.ci.core.config.registry.TaskConfigYamlParser;
import ru.yandex.ci.core.config.registry.TaskId;
import ru.yandex.ci.core.job.JobResource;
import ru.yandex.ci.core.launch.FlowReference;
import ru.yandex.ci.core.proto.ProtobufSerialization;
import ru.yandex.ci.core.tasklet.SchemaOptions;
import ru.yandex.ci.core.tasklet.SchemaService;
import ru.yandex.ci.core.tasklet.TaskletMetadata;
import ru.yandex.ci.core.tasklet.TaskletMetadataService;
import ru.yandex.ci.core.tasklet.TaskletRuntime;
import ru.yandex.ci.core.taskletv2.TaskletV2MetadataService;
import ru.yandex.ci.engine.test.schema.EmptyData;
import ru.yandex.ci.engine.test.schema.PrimitiveAndRepeatedData;
import ru.yandex.ci.engine.test.schema.TaskletWithPrimitiveAndRepeatedData;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.definition.job.Job;
import ru.yandex.ci.flow.engine.runtime.JobExecutorObjectProvider;
import ru.yandex.ci.flow.engine.source_code.SourceCodeProvider;
import ru.yandex.ci.flow.engine.source_code.impl.ReflectionsSourceCodeProvider;
import ru.yandex.ci.flow.engine.source_code.impl.SourceCodeServiceImpl;
import ru.yandex.ci.test.TestUtils;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;
import static ru.yandex.ci.core.tasklet.TaskletMetadataHelper.metadataFor;

@Slf4j
@ExtendWith(MockitoExtension.class)
class FlowFactoryTest extends CommonTestBase {
    @Mock
    private TaskletMetadataService taskletMetadataService;
    @Mock
    private TaskletV2MetadataService taskletV2MetadataService;
    @Mock
    private JobExecutorObjectProvider jobExecutorObjectProvider;

    private FlowFactory flowFactory;
    private SchemaService schemaService;

    @BeforeEach
    public void setUp() {
        SourceCodeProvider sourceCodeProvider = new ReflectionsSourceCodeProvider(
                ReflectionsSourceCodeProvider.SOURCE_CODE_PACKAGE
        );
        var sourceCodeService = new SourceCodeServiceImpl(
                sourceCodeProvider,
                jobExecutorObjectProvider,
                null,
                false,
                null
        );

        schemaService = new SchemaService();
        flowFactory = new FlowFactory(
                taskletMetadataService,
                taskletV2MetadataService,
                schemaService,
                sourceCodeService
        );
    }

    @Test
    void resourceFromAYaml() throws Exception {
        testMerge(
                "task/tasklet-with-basic-input.yaml",
                "flow-with-input",
                """
                        {
                            "primitive_values": {
                                "int_field": 10,
                                "double_field": 20.1
                            },
                            "values_yaml": {
                                "other_string": "three"
                            },
                            "simple_data_field": {
                                "simple_string": ["one", "two", "three"]
                            },
                            "values_registry": {
                                "simple_string": "none"
                            }
                        }
                        """,
                """
                            primitive_values: {
                                int_field: 10
                                double_field: 20.1
                            },
                            values_yaml: {
                                other_string: "three"
                            },
                            simple_data_field: {
                                simple_string: ["one", "two", "three"]
                            },
                            values_registry: {
                                simple_string: "none"
                            }
                        """);
    }

    @Test
    void resourceFromAYamlCamelCase() throws Exception {
        testMerge(
                "task/tasklet-with-basic-input.yaml",
                "flow-with-input-camel-case",
                """
                        {
                            "primitive_values": {
                                "int_field": 10,
                                "double_field": 20.1
                            },
                            "values_yaml": {
                                "other_string": "three"
                            },
                            "simple_data_field": {
                                "simple_string": ["one", "two", "three"]
                            },
                            "values_registry": {
                                "simple_string": "none"
                            }
                        }
                        """,
                """
                            primitive_values: {
                                int_field: 10
                                double_field: 20.1
                            },
                            values_yaml: {
                                other_string: "three"
                            },
                            simple_data_field: {
                                simple_string: ["one", "two", "three"]
                            },
                            values_registry: {
                                simple_string: "none"
                            }
                        """);
    }

    @Test
    void resourceFromRegistry() throws Exception {
        testMerge(
                "task/tasklet-with-full-input.yaml",
                "flow-simple",
                """
                        {
                            "primitive_values": {
                                "int_field": 1,
                                "string_field": "Test value"
                            },
                            "values_yaml": {
                                "other_string": "three"
                            },
                            "simple_data_field": {
                                "simple_string": ["three", "five"]
                            },
                            "values_registry": {
                                "simple_string": "two"
                            }
                        }
                        """,
                """
                            primitive_values: {
                                int_field: 1
                                string_field: "Test value"
                            },
                            values_yaml: {
                                other_string: "three"
                            },
                            simple_data_field: {
                                simple_string: ["three", "five"]
                            },
                            values_registry: {
                                simple_string: "two"
                            }
                        """);
    }

    @Test
    void resourceMergedSameCases() throws Exception {
        testMerge(
                "task/tasklet-with-full-input.yaml",
                "flow-with-input-camel-case",
                """
                        {
                            "primitive_values": {
                                "int_field": 10,
                                "double_field": 20.1,
                                "string_field": "Test value"
                            },
                            "values_yaml": {
                                "other_string": "three"
                            },
                            "simple_data_field": {
                                "simple_string": ["one", "two", "three"]
                            },
                            "values_registry": {
                                "simple_string": "two"
                            }
                        }
                        """,
                """
                            primitive_values: {
                                int_field: 10
                                double_field: 20.1
                                string_field: "Test value"
                            },
                            values_yaml: {
                                other_string: "three"
                            },
                            simple_data_field: {
                                simple_string: ["one", "two", "three"]
                            },
                            values_registry: {
                                simple_string: "two"
                            }
                        """);
    }

    @Test
    void resourceMergedDifferentCases() throws Exception {
        testMerge(
                "task/tasklet-with-full-input.yaml",
                "flow-with-input",
                """
                        {
                            "primitive_values": {
                                "int_field": 10,
                                "double_field": 20.1,
                                "string_field": "Test value"
                            },
                            "values_yaml": {
                                "other_string": "three"
                            },
                            "simple_data_field": {
                                "simple_string": ["one", "two", "three"]
                            },
                            "values_registry": {
                                "simple_string": "two"
                            }
                        }
                        """,
                """
                            primitive_values: {
                                int_field: 10
                                double_field: 20.1
                                string_field: "Test value"
                            },
                            values_yaml: {
                                other_string: "three"
                            },
                            simple_data_field: {
                                simple_string: ["one", "two", "three"]
                            },
                            values_registry: {
                                simple_string: "two"
                            }
                        """);
    }

    @Test
    void resourceMergedDifferentCasesInternal() {

        var jobError = "Unable to parse protobuf input message ci.test.PrimitiveAndRepeatedData: " +
                "Field ci.test.PrimitiveInput.int_field has already been set.";
        var validationReport = ValidationReport.builder()
                .flowReport(new ValidationReport.FlowReport(Map.of("job", Set.of(jobError))))
                .build();

        assertThatThrownBy(() ->
                testMerge(
                        "task/tasklet-with-full-input.yaml",
                        "flow-with-input-camel-case-internal",
                        "",
                        ""))
                .isInstanceOf(AYamlValidationException.class)
                .hasMessage(validationReport.toString());
    }

    @Test
    void createFlowWithImplicitStage() throws Exception {
        var id = TaskletMetadata.Id.of("WoodcutterPy", TaskletRuntime.SANDBOX, 31241241L);
        TaskletMetadata metadata = TaskletMetadata.builder()
                .id(id)
                .build();

        TaskConfig taskConfig = TaskConfigYamlParser.parse(TestUtils.textResource("task/tasklet-simple.yaml"));

        String path = "flows/flow-with-implicit-stage/a.yaml";
        AYamlConfig config = AYamlParser.parseAndValidate(TestUtils.textResource(path)).getConfig();
        Map<TaskId, TaskConfig> taskConfigs = Map.of(TaskId.of("ci_test/some_task"), taskConfig);
        CiProcessId processId = CiProcessId.ofRelease(Path.of(path), "my-release");

        Mockito.when(taskletMetadataService.fetchMetadata(id)).thenReturn(metadata);
        Flow flow = flowFactory.create(processId, config, taskConfigs, FlowCustomizedConfig.EMPTY);

        Job job = flow.getJobs().get(0);
        assertThat(job.getStage()).isNotNull();
        assertThat(job.getStage().getId()).isEqualTo("single");
        assertThat(job.getStage().getTitle()).isEqualTo("Single");
    }

    @Test
    void createFlowWithSingleStage() throws Exception {
        var id = TaskletMetadata.Id.of("WoodcutterPy", TaskletRuntime.SANDBOX, 31241241L);
        TaskletMetadata metadata = TaskletMetadata.builder()
                .id(id)
                .build();

        TaskConfig taskConfig = TaskConfigYamlParser.parse(TestUtils.textResource("task/tasklet-simple.yaml"));

        String path = "flows/flow-with-single-stage/a.yaml";
        AYamlConfig config = AYamlParser.parseAndValidate(TestUtils.textResource(path)).getConfig();
        Map<TaskId, TaskConfig> taskConfigs = Map.of(TaskId.of("ci_test/some_task"), taskConfig);
        CiProcessId processId = CiProcessId.ofRelease(Path.of(path), "my-release");

        Mockito.when(taskletMetadataService.fetchMetadata(id)).thenReturn(metadata);
        Flow flow = flowFactory.create(processId, config, taskConfigs, FlowCustomizedConfig.EMPTY);

        Job job = flow.getJobs().get(0);
        assertThat(job.getStage()).isNotNull();
        assertThat(job.getStage().getId()).isEqualTo("build-stage");
        assertThat(job.getStage().getTitle()).isEqualTo("Build");
    }

    @Test
    void createFlowWithHotfix() throws Exception {
        var id = TaskletMetadata.Id.of("WoodcutterPy", TaskletRuntime.SANDBOX, 31241241L);
        TaskletMetadata metadata = TaskletMetadata.builder()
                .id(id)
                .build();

        TaskConfig taskConfig = TaskConfigYamlParser.parse(TestUtils.textResource("task/tasklet-simple.yaml"));

        String path = "flows/flow-with-hotfix/a.yaml";
        AYamlConfig config = AYamlParser.parseAndValidate(TestUtils.textResource(path)).getConfig();
        Map<TaskId, TaskConfig> taskConfigs = Map.of(TaskId.of("ci_test/some_task"), taskConfig);
        CiProcessId processId = CiProcessId.ofRelease(Path.of(path), "my-release");

        Mockito.when(taskletMetadataService.fetchMetadata(id)).thenReturn(metadata);

        Flow flow = flowFactory.create(processId, config, taskConfigs,
                FlowCustomizedConfig.builder()
                        .flowReference(FlowReference.of("flow-with-hotfix", Common.FlowType.FT_HOTFIX))
                        .build()
        );

        Job job = flow.getJobs().get(0);
        assertThat(job.getStage()).isNotNull();
        assertThat(job.getStage().getId()).isEqualTo("build-stage");
        assertThat(job.getStage().getTitle()).isEqualTo("Build");

        assertThat(job.getTitle()).isEqualTo("build hotfix, from variable");
    }

    @Test
    void createFlowWithHotfix2() throws Exception {
        var id = TaskletMetadata.Id.of("WoodcutterPy", TaskletRuntime.SANDBOX, 31241241L);
        TaskletMetadata metadata = TaskletMetadata.builder()
                .id(id)
                .build();

        TaskConfig taskConfig = TaskConfigYamlParser.parse(TestUtils.textResource("task/tasklet-simple.yaml"));

        String path = "flows/flow-with-hotfix/a.yaml";
        AYamlConfig config = AYamlParser.parseAndValidate(TestUtils.textResource(path)).getConfig();
        Map<TaskId, TaskConfig> taskConfigs = Map.of(TaskId.of("ci_test/some_task"), taskConfig);
        CiProcessId processId = CiProcessId.ofRelease(Path.of(path), "my-release2");

        Mockito.when(taskletMetadataService.fetchMetadata(id)).thenReturn(metadata);

        Flow flow = flowFactory.create(processId, config, taskConfigs,
                FlowCustomizedConfig.builder()
                        .flowReference(FlowReference.of("flow-with-hotfix", Common.FlowType.FT_HOTFIX))
                        .build()
        );

        Job job = flow.getJobs().get(0);
        assertThat(job.getStage()).isNotNull();
        assertThat(job.getStage().getId()).isEqualTo("build-stage");
        assertThat(job.getStage().getTitle()).isEqualTo("Build");

        assertThat(job.getTitle()).isEqualTo("build hotfix, from hotfix");
    }


    @Test
    void createFlowWithRollback() throws Exception {
        var id = TaskletMetadata.Id.of("WoodcutterPy", TaskletRuntime.SANDBOX, 31241241L);
        TaskletMetadata metadata = TaskletMetadata.builder()
                .id(id)
                .build();

        TaskConfig taskConfig = TaskConfigYamlParser.parse(TestUtils.textResource("task/tasklet-simple.yaml"));

        String path = "flows/flow-with-hotfix/a.yaml";
        AYamlConfig config = AYamlParser.parseAndValidate(TestUtils.textResource(path)).getConfig();
        Map<TaskId, TaskConfig> taskConfigs = Map.of(TaskId.of("ci_test/some_task"), taskConfig);
        CiProcessId processId = CiProcessId.ofRelease(Path.of(path), "my-release");

        Mockito.when(taskletMetadataService.fetchMetadata(id)).thenReturn(metadata);

        Flow flow = flowFactory.create(processId, config, taskConfigs,
                FlowCustomizedConfig.builder()
                        .flowReference(FlowReference.of("flow-with-rollback", Common.FlowType.FT_ROLLBACK))
                        .build()
        );

        Job job = flow.getJobs().get(0);
        assertThat(job.getStage()).isNotNull();
        assertThat(job.getStage().getId()).isEqualTo("build-stage");
        assertThat(job.getStage().getTitle()).isEqualTo("Build");

        assertThat(job.getTitle()).isEqualTo("build rollback, from variable");
    }

    @Test
    void createFlowWithRollback2() throws Exception {
        var id = TaskletMetadata.Id.of("WoodcutterPy", TaskletRuntime.SANDBOX, 31241241L);
        TaskletMetadata metadata = TaskletMetadata.builder()
                .id(id)
                .build();

        TaskConfig taskConfig = TaskConfigYamlParser.parse(TestUtils.textResource("task/tasklet-simple.yaml"));

        String path = "flows/flow-with-hotfix/a.yaml";
        AYamlConfig config = AYamlParser.parseAndValidate(TestUtils.textResource(path)).getConfig();
        Map<TaskId, TaskConfig> taskConfigs = Map.of(TaskId.of("ci_test/some_task"), taskConfig);
        CiProcessId processId = CiProcessId.ofRelease(Path.of(path), "my-release2");

        Mockito.when(taskletMetadataService.fetchMetadata(id)).thenReturn(metadata);

        Flow flow = flowFactory.create(processId, config, taskConfigs,
                FlowCustomizedConfig.builder()
                        .flowReference(FlowReference.of("flow-with-rollback", Common.FlowType.FT_ROLLBACK))
                        .build()
        );

        Job job = flow.getJobs().get(0);
        assertThat(job.getStage()).isNotNull();
        assertThat(job.getStage().getId()).isEqualTo("build-stage");
        assertThat(job.getStage().getTitle()).isEqualTo("Build");

        assertThat(job.getTitle()).isEqualTo("build rollback, from rollback");
    }

    @Test
    void createFlowWithInputForInternalTaskValid() throws Exception {
        var taskConfig = TaskConfigYamlParser.parse(
                TestUtils.textResource("test-repo/r2/ci/registry/demo/internal/create_issue_test.yaml"));

        String path = "flows/flow-with-internal-task-valid/a.yaml";
        var config = AYamlParser.parseAndValidate(TestUtils.textResource(path)).getConfig();
        var taskConfigs = Map.of(TaskId.of("ci_test/some_task"), taskConfig);

        var processId = CiProcessId.ofRelease(Path.of(path), "my-release");
        var flow = flowFactory.create(processId, config, taskConfigs, FlowCustomizedConfig.EMPTY);

        Job job = flow.getJobs().get(0);
        assertThat(job.getStage()).isNotNull();
        assertThat(job.getStage().getId()).isEqualTo("build-stage");
        assertThat(job.getStage().getTitle()).isEqualTo("Build");
    }

    @Test
    void createFlowWithInputForInternalTaskInvalid() throws Exception {
        var taskConfig = TaskConfigYamlParser.parse(
                TestUtils.textResource("test-repo/r2/ci/registry/demo/internal/create_issue_test.yaml"));

        String path = "flows/flow-with-internal-task-invalid/a.yaml";
        var config = AYamlParser.parseAndValidate(TestUtils.textResource(path)).getConfig();
        var taskConfigs = Map.of(TaskId.of("ci_test/some_task"), taskConfig);

        var processId = CiProcessId.ofRelease(Path.of(path), "my-release");

        assertThatThrownBy(() -> flowFactory.create(processId, config, taskConfigs, FlowCustomizedConfig.EMPTY))
                .isInstanceOf(AYamlValidationException.class)
                .hasMessageContaining("Unable to parse protobuf input message ci.tracker.create_issue.Template: " +
                        "Cannot find field: update_template in message ci.tracker.create_issue.Template");
    }

    private void testMerge(
            String taskletPath,
            String flowName,
            String expectJson,
            String expectProto
    ) throws Exception {
        var metadata = metadataFor(
                "WoodcutterPy",
                31241241L,
                TaskletWithPrimitiveAndRepeatedData.getDescriptor(),
                PrimitiveAndRepeatedData.getDescriptor(),
                EmptyData.getDescriptor()
        );
        Mockito.when(taskletMetadataService.fetchMetadata(metadata.getId())).thenReturn(metadata);

        var path = "flows/flow-with-input/a.yaml";

        var processId = CiProcessId.ofFlow(FlowFullId.of(Path.of(path), flowName));
        var config = AYamlParser.parseAndValidate(TestUtils.textResource(path)).getConfig();
        var taskConfigs = Map.of(
                TaskId.of("ci_test/some_task"),
                TaskConfigYamlParser.parse(TestUtils.textResource(taskletPath))
        );

        var flow = flowFactory.create(processId, config, taskConfigs, FlowCustomizedConfig.EMPTY);
        var job = flow.getJobs().get(0);

        for (var resource : job.getStaticResources()) {
            log.info("Resource: {}, {}, {}",
                    resource.getResourceType(), resource.getParentField(), resource.getData());
        }

        var resources = job.getStaticResources().stream()
                .map(JobResource::of)
                .toList();
        var actualJson = schemaService.composeInput(metadata, SchemaOptions.defaultOptions(), resources);

        assertThat(actualJson)
                .describedAs("Json are not the same")
                .isEqualTo(JsonParser.parseString(expectJson));

        var actualProtoBuilder = ProtobufSerialization.deserializeFromGson(actualJson,
                PrimitiveAndRepeatedData.newBuilder());

        var expectProtoObject = TestUtils.parseProtoTextFromString(expectProto, PrimitiveAndRepeatedData.class);

        assertThat(actualProtoBuilder.build())
                .isEqualTo(expectProtoObject);
    }

}
