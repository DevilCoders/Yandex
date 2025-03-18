package ru.yandex.ci.engine.config.validation;

import java.nio.file.Path;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;
import java.util.stream.IntStream;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.github.fge.jsonschema.core.exceptions.ProcessingException;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.Mockito;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.mock.mockito.SpyBean;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.core.CoreYdbTestBase;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.a.AYamlParser;
import ru.yandex.ci.core.config.a.model.AYamlConfig;
import ru.yandex.ci.core.config.registry.TaskConfig;
import ru.yandex.ci.core.config.registry.TaskConfigYamlParser;
import ru.yandex.ci.core.config.registry.TaskId;
import ru.yandex.ci.core.config.registry.Type;
import ru.yandex.ci.core.launch.FlowReference;
import ru.yandex.ci.core.tasklet.Features;
import ru.yandex.ci.core.tasklet.TaskletMetadata;
import ru.yandex.ci.core.tasklet.TaskletMetadataService;
import ru.yandex.ci.core.tasklet.TaskletMetadataValidationException;
import ru.yandex.ci.core.tasklet.TaskletRuntime;
import ru.yandex.ci.engine.config.validation.InputOutputTaskValidator.FlowViolations;
import ru.yandex.ci.engine.config.validation.InputOutputTaskValidator.ReportId;
import ru.yandex.ci.engine.spring.TaskValidatorConfig;
import ru.yandex.ci.engine.test.schema.DifferentInputsData;
import ru.yandex.ci.engine.test.schema.DifferentInputsTasklet;
import ru.yandex.ci.engine.test.schema.EmptyData;
import ru.yandex.ci.engine.test.schema.MapContainer;
import ru.yandex.ci.engine.test.schema.MapInput;
import ru.yandex.ci.engine.test.schema.MapOutput;
import ru.yandex.ci.engine.test.schema.OneOfInput;
import ru.yandex.ci.engine.test.schema.OneOfTasklet;
import ru.yandex.ci.engine.test.schema.OtherData;
import ru.yandex.ci.engine.test.schema.PrimitiveInput;
import ru.yandex.ci.engine.test.schema.PrimitiveInputData;
import ru.yandex.ci.engine.test.schema.PrimitiveMapContainer;
import ru.yandex.ci.engine.test.schema.PrimitiveOutput;
import ru.yandex.ci.engine.test.schema.SimpleData;
import ru.yandex.ci.engine.test.schema.SimpleInputListTasklet;
import ru.yandex.ci.engine.test.schema.SimpleListData;
import ru.yandex.ci.engine.test.schema.SimpleNoInputTasklet;
import ru.yandex.ci.engine.test.schema.SimpleNoOutputTasklet;
import ru.yandex.ci.engine.test.schema.SimpleOtherOutputTasklet;
import ru.yandex.ci.engine.test.schema.SimpleRepeatedData;
import ru.yandex.ci.engine.test.schema.SimpleRepeatedTasklet;
import ru.yandex.ci.engine.test.schema.SimpleTasklet;
import ru.yandex.ci.engine.test.schema.TaskletWithMapContainer;
import ru.yandex.ci.engine.test.schema.TaskletWithMapInput;
import ru.yandex.ci.engine.test.schema.TaskletWithMapOutput;
import ru.yandex.ci.engine.test.schema.TaskletWithPrimitiveInput;
import ru.yandex.ci.engine.test.schema.TaskletWithPrimitiveInputData;
import ru.yandex.ci.engine.test.schema.TaskletWithPrimitiveMapContainer;
import ru.yandex.ci.flow.engine.source_code.SourceCodeService;
import ru.yandex.ci.flow.engine.source_code.impl.SourceCodeServiceImpl;
import ru.yandex.ci.test.TestUtils;

import static org.assertj.core.api.Assertions.assertThat;
import static ru.yandex.ci.core.tasklet.TaskletMetadataHelper.metadataFor;

@ContextConfiguration(classes = {
        TaskValidatorConfig.class
})
class InputOutputTaskValidatorTest extends CoreYdbTestBase {

    @SpyBean
    protected TaskletMetadataService taskletMetadataService;

    //

    @Autowired
    private SourceCodeService sourceCodeService;

    @Autowired
    private InputOutputTaskValidator validator;

    @BeforeEach
    void setUp() {
        ((SourceCodeServiceImpl) sourceCodeService).reset();
    }

    @Test
    void validateJobWithSingleStaticResource() throws Exception {
        var report = validateSingleTasklet("flows/validation/job-with-static-resource.yaml");
        assertThat(report.isValid()).isTrue();
    }

    @Test
    void validateJobWithUpstreamResource() throws Exception {
        AYamlConfig config = AYamlParser.parseAndValidate(
                TestUtils.textResource("flows/validation/job-with-upstream.yaml")
        ).getConfig();
        Map<TaskId, TaskConfig> taskConfigs = Map.of(
                TaskId.of("ci_test/some_task_1"),
                TaskConfigYamlParser.parse(TestUtils.textResource("task/validation/some-task-1.yaml")),
                TaskId.of("ci_test/some_task_2"),
                TaskConfigYamlParser.parse(TestUtils.textResource("task/validation/some-task-2.yaml"))
        );

        Mockito.doReturn(metadataFor(
                "implementation1",
                1L,
                SimpleNoInputTasklet.getDescriptor(),
                EmptyData.getDescriptor(),
                SimpleData.getDescriptor()
        )).when(taskletMetadataService).fetchMetadata(
                TaskletMetadata.Id.of("implementation1", TaskletRuntime.SANDBOX, 1L)
        );
        Mockito.doReturn(metadataFor(
                "implementation2",
                2L,
                SimpleTasklet.getDescriptor(),
                SimpleData.getDescriptor(),
                SimpleData.getDescriptor()
        )).when(taskletMetadataService).fetchMetadata(
                TaskletMetadata.Id.of("implementation2", TaskletRuntime.SANDBOX, 2L)
        );

        InputOutputTaskValidator.Report report = validator.validate(
                Path.of("flows/validation/a.yaml"),
                config,
                taskConfigs
        );

        assertThat(report.isValid()).isTrue();
    }

    @Test
    void validateJobWithMultipleUpstreamSameTypeResourcesSingle() throws Exception {
        AYamlConfig config = AYamlParser.parseAndValidate(
                TestUtils.textResource("flows/validation/job-with-multiple-upstreams-same-type-single.yaml")
        ).getConfig();
        Map<TaskId, TaskConfig> taskConfigs = Map.of(
                TaskId.of("ci_test/some_task_1"),
                TaskConfigYamlParser.parse(TestUtils.textResource("task/validation/some-task-1.yaml")),
                TaskId.of("ci_test/some_task_2"),
                TaskConfigYamlParser.parse(TestUtils.textResource("task/validation/some-task-2.yaml"))
        );

        Mockito.doReturn(metadataFor(
                "implementation1",
                1L,
                SimpleNoInputTasklet.getDescriptor(),
                EmptyData.getDescriptor(),
                SimpleData.getDescriptor()
        )).when(taskletMetadataService).fetchMetadata(
                TaskletMetadata.Id.of("implementation1", TaskletRuntime.SANDBOX, 1L)
        );
        Mockito.doReturn(metadataFor(
                "implementation2",
                2L,
                SimpleTasklet.getDescriptor(),
                SimpleData.getDescriptor(),
                SimpleData.getDescriptor()
        )).when(taskletMetadataService).fetchMetadata(
                TaskletMetadata.Id.of("implementation2", TaskletRuntime.SANDBOX, 2L)
        );

        InputOutputTaskValidator.Report report = validator.validate(
                Path.of("flows/validation/a.yaml"),
                config,
                taskConfigs
        );

        assertThat(report.isValid()).isTrue();
    }

    @Test
    void validateJobWithMultipleUpstreamSameTypeResourcesMulti() throws Exception {
        var metadata1 = metadataFor(
                "implementation1",
                1L,
                SimpleNoInputTasklet.getDescriptor(),
                EmptyData.getDescriptor(),
                SimpleData.getDescriptor()
        );
        var metadata2 = metadataFor(
                "implementation2",
                2L,
                SimpleInputListTasklet.getDescriptor(),
                SimpleListData.getDescriptor(),
                SimpleData.getDescriptor()
        );

        var report = validateFlowWith2Tasks(
                "flows/validation/job-with-multiple-upstreams-same-type-multi.yaml",
                metadata1,
                metadata2
        );

        assertThat(report.isValid()).isTrue();
    }

    @Test
    void validateJobWithMultipleUpstreamResources() throws Exception {
        AYamlConfig config = AYamlParser.parseAndValidate(
                TestUtils.textResource("flows/validation/job-with-multiple-upstreams.yaml")
        ).getConfig();
        Map<TaskId, TaskConfig> taskConfigs = Map.of(
                TaskId.of("ci_test/some_task_1"),
                TaskConfigYamlParser.parse(TestUtils.textResource("task/validation/some-task-1.yaml")),
                TaskId.of("ci_test/some_task_2"),
                TaskConfigYamlParser.parse(TestUtils.textResource("task/validation/some-task-2.yaml")),
                TaskId.of("ci_test/some_task_3"),
                TaskConfigYamlParser.parse(TestUtils.textResource("task/validation/some-task-3.yaml"))
        );

        Mockito.doReturn(metadataFor(
                "implementation1",
                1L,
                SimpleNoInputTasklet.getDescriptor(),
                EmptyData.getDescriptor(),
                SimpleData.getDescriptor()
        )).when(taskletMetadataService).fetchMetadata(
                TaskletMetadata.Id.of("implementation1", TaskletRuntime.SANDBOX, 1L)
        );
        Mockito.doReturn(metadataFor(
                "implementation2",
                2L,
                SimpleTasklet.getDescriptor(),
                SimpleData.getDescriptor(),
                SimpleData.getDescriptor()
        )).when(taskletMetadataService).fetchMetadata(
                TaskletMetadata.Id.of("implementation2", TaskletRuntime.SANDBOX, 2L)
        );
        Mockito.doReturn(metadataFor(
                "implementation3",
                3L,
                SimpleInputListTasklet.getDescriptor(),
                SimpleListData.getDescriptor(),
                SimpleData.getDescriptor()
        )).when(taskletMetadataService).fetchMetadata(
                TaskletMetadata.Id.of("implementation3", TaskletRuntime.SANDBOX, 3L)
        );

        InputOutputTaskValidator.Report report = validator.validate(
                Path.of("flows/validation/a.yaml"),
                config,
                taskConfigs
        );

        assertThat(report.isValid()).isTrue();
    }

    @Test
    void validateJobWithOtherUpstreamResource() throws Exception {
        AYamlConfig config = AYamlParser.parseAndValidate(
                TestUtils.textResource("flows/validation/job-with-upstream.yaml")
        ).getConfig();
        Map<TaskId, TaskConfig> taskConfigs = Map.of(
                TaskId.of("ci_test/some_task_1"),
                TaskConfigYamlParser.parse(TestUtils.textResource("task/validation/some-task-1.yaml")),
                TaskId.of("ci_test/some_task_2"),
                TaskConfigYamlParser.parse(TestUtils.textResource("task/validation/some-task-2.yaml"))
        );

        Mockito.doReturn(metadataFor(
                "implementation1",
                1L,
                SimpleOtherOutputTasklet.getDescriptor(),
                EmptyData.getDescriptor(),
                OtherData.getDescriptor()
        )).when(taskletMetadataService).fetchMetadata(
                TaskletMetadata.Id.of("implementation1", TaskletRuntime.SANDBOX, 1L)
        );
        Mockito.doReturn(metadataFor(
                "implementation2",
                2L,
                SimpleTasklet.getDescriptor(),
                SimpleData.getDescriptor(),
                SimpleData.getDescriptor()
        )).when(taskletMetadataService).fetchMetadata(
                TaskletMetadata.Id.of("implementation2", TaskletRuntime.SANDBOX, 2L)
        );

        InputOutputTaskValidator.Report report = validator.validate(
                Path.of("flows/validation/a.yaml"),
                config,
                taskConfigs
        );

        ReportId flowId = reportId("flow-with-upstream", CiProcessId.Type.FLOW);

        assertThat(report.isValid()).isFalse();
        assertThat(report.getViolationsByFlows()).containsOnlyKeys(flowId);
        assertThat(report.getViolationsByFlows().get(flowId).getViolationsByJobs())
                .isEqualTo(
                        Map.of("job-with-input-2",
                                Set.of("resources of following types are missed: ci.test.Simple, actual received " +
                                        "resources: "))
                );
    }

    @Test
    void validateJobWithUnexpectedListUpstream() throws Exception {
        AYamlConfig config = AYamlParser.parseAndValidate(
                TestUtils.textResource("flows/validation/job-with-multiple-upstreams.yaml")
        ).getConfig();
        Map<TaskId, TaskConfig> taskConfigs = Map.of(
                TaskId.of("ci_test/some_task_1"),
                TaskConfigYamlParser.parse(TestUtils.textResource("task/validation/some-task-1.yaml")),
                TaskId.of("ci_test/some_task_2"),
                TaskConfigYamlParser.parse(TestUtils.textResource("task/validation/some-task-2.yaml")),
                TaskId.of("ci_test/some_task_3"),
                TaskConfigYamlParser.parse(TestUtils.textResource("task/validation/some-task-3.yaml"))
        );

        Mockito.doReturn(metadataFor(
                "implementation1",
                1L,
                SimpleNoInputTasklet.getDescriptor(),
                EmptyData.getDescriptor(),
                SimpleData.getDescriptor()
        )).when(taskletMetadataService).fetchMetadata(
                TaskletMetadata.Id.of("implementation1", TaskletRuntime.SANDBOX, 1L)
        );
        Mockito.doReturn(metadataFor(
                "implementation2",
                2L,
                SimpleTasklet.getDescriptor(),
                SimpleData.getDescriptor(),
                SimpleData.getDescriptor()
        )).when(taskletMetadataService).fetchMetadata(
                TaskletMetadata.Id.of("implementation2", TaskletRuntime.SANDBOX, 2L)
        );
        Mockito.doReturn(metadataFor(
                "implementation3",
                3L,
                SimpleTasklet.getDescriptor(),
                SimpleData.getDescriptor(),
                SimpleData.getDescriptor()
        )).when(taskletMetadataService).fetchMetadata(
                TaskletMetadata.Id.of("implementation3", TaskletRuntime.SANDBOX, 3L)
        );

        InputOutputTaskValidator.Report report = validator.validate(
                Path.of("flows/validation/a.yaml"),
                config,
                taskConfigs
        );

        ReportId flowId = reportId("flow-with-multiple-upstreams", CiProcessId.Type.FLOW);

        assertThat(report.isValid()).isFalse();
        assertThat(report.getViolationsByFlows()).containsOnlyKeys(flowId);
        assertThat(report.getViolationsByFlows().get(flowId).getViolationsByJobs())
                .isEqualTo(Map.of("job-with-input-3", Set.of(
                        "unexpected multiple resources of following types: " +
                                "ci.test.Simple (got from job-with-input-1, job-with-input-2)"
                )));
    }

    @Test
    void validateJobWithUnexpectedVersion() throws Exception {
        AYamlConfig config = AYamlParser.parseAndValidate(
                TestUtils.textResource("flows/validation/job-with-unexpected-version.yaml")
        ).getConfig();
        Map<TaskId, TaskConfig> taskConfigs = Map.of(
                TaskId.of("ci_test/some_task_1"),
                TaskConfigYamlParser.parse(TestUtils.textResource("task/validation/some-task-1.yaml"))
        );

        Mockito.doReturn(metadataFor(
                "implementation1",
                1L,
                SimpleNoInputTasklet.getDescriptor(),
                EmptyData.getDescriptor(),
                SimpleData.getDescriptor()
        )).when(taskletMetadataService).fetchMetadata(
                TaskletMetadata.Id.of("implementation1", TaskletRuntime.SANDBOX, 1L)
        );

        InputOutputTaskValidator.Report report = validator.validate(
                Path.of("flows/validation/a.yaml"),
                config,
                taskConfigs
        );

        ReportId flowId = reportId("job-with-unexpected-version", CiProcessId.Type.FLOW);

        assertThat(report.isValid()).isFalse();
        assertThat(report.getViolationsByFlows()).containsOnlyKeys(flowId);
        assertThat(report.getViolationsByFlows().get(flowId).getViolationsByJobs())
                .isEqualTo(Map.of("job", Set.of(
                        "not found version 'missing' for job 'job (deadly)'. Available versions: stable"
                )));
    }

    @Test
    void validateJobWithInvalidStaticResource() throws Exception {
        var report = validateSingleTasklet("flows/validation/job-with-invalid-context-input.yaml");

        ReportId flowId = reportId("flow-with-static-resource-context", CiProcessId.Type.FLOW);

        assertThat(report.isValid()).isFalse();
        assertThat(report.getViolationsByFlows()).containsOnlyKeys(flowId);
        assertThat(report.getViolationsByFlows().get(flowId).getViolationsByJobs())
                .isEqualTo(Map.of("job-with-static-resource-context", Set.of(
                        "Context input is prohibited for " + Type.TASKLET
                )));
    }

    @Test
    void validateJobWithInvalidPrimitiveIntResource() throws Exception {
        var report = validateSingleTasklet("flows/validation/job-with-invalid-primitive-int-resource.yaml");

        ReportId flowId = reportId("flow-with-static-resource-context", CiProcessId.Type.FLOW);

        assertThat(report.isValid()).isFalse();
        assertThat(report.getViolationsByFlows()).containsOnlyKeys(flowId);
        assertThat(report.getViolationsByFlows().get(flowId).getViolationsByJobs())
                .isEqualTo(Map.of("job-with-static-primitive-resource", Set.of(
                        """
                                Unable to parse protobuf input message ci.test.PrimitiveInputData: \
                                Not an int32 value: "Int ${tasks.job-1-x.int_field}\""""
                )));
    }

    @Test
    void validateJobWithInvalidPrimitiveDoubleResource() throws Exception {
        var report = validateSingleTasklet("flows/validation/job-with-invalid-primitive-double-resource.yaml");

        ReportId flowId = reportId("flow-with-static-resource-context", CiProcessId.Type.FLOW);

        assertThat(report.isValid()).isFalse();
        assertThat(report.getViolationsByFlows()).containsOnlyKeys(flowId);
        assertThat(report.getViolationsByFlows().get(flowId).getViolationsByJobs())
                .isEqualTo(Map.of("job-with-static-primitive-resource", Set.of(
                        """
                                Unable to parse protobuf input message ci.test.PrimitiveInputData: \
                                Not an double value: "Double ${tasks.job-1-x.double_field}\""""
                )));
    }

    @Test
    void validateJobWithInvalidPrimitiveBooleanResource() throws Exception {
        var report = validateSingleTasklet("flows/validation/job-with-invalid-primitive-boolean-resource.yaml");

        ReportId flowId = reportId("flow-with-static-resource-context", CiProcessId.Type.FLOW);

        assertThat(report.isValid()).isFalse();
        assertThat(report.getViolationsByFlows()).containsOnlyKeys(flowId);
        assertThat(report.getViolationsByFlows().get(flowId).getViolationsByJobs())
                .isEqualTo(Map.of("job-with-static-primitive-resource", Set.of("""
                        Unable to parse protobuf input message ci.test.PrimitiveInputData: \
                        Invalid bool value: "Boolean ${tasks.job-1-x.boolean_field}\""""
                )));
    }

    @Test
    void validateJobWithInvalidExpression() throws Exception {
        var report = validateSingleTasklet("flows/validation/job-with-invalid-expression.yaml");

        ReportId flowId = reportId("flow-with-invalid-expression", CiProcessId.Type.FLOW);

        assertThat(report.isValid()).isFalse();
        assertThat(report.getViolationsByFlows()).containsOnlyKeys(flowId);
        assertThat(report.getViolationsByFlows().get(flowId).getViolationsByJobs())
                .isEqualTo(Map.of("job-with-invalid-expression", Set.of("""
                        Unable to parse protobuf input message ci.test.SimpleData: \
                        Unable to substitute field "simple_data_field.simpleString" with expression \
                        "${a + b}": Unable to compile expression "a + b": \
                        syntax error token recognition error at: '+' at position 2, \
                        syntax error extraneous input 'b' expecting <EOF> at position 4"""
                )));
    }

    @Test
    void validateJobWithMultiplyCrossStageAuto() throws Exception {
        var report = validateSingleTasklet("flows/validation/job-with-multiply-cross-stage-auto.yaml");
        assertThat(report.isValid()).isTrue();
    }

    @Test
    void outdatedTasklets() throws Exception {
        AYamlConfig config = AYamlParser.parseAndValidate(TestUtils.textResource("flows/validation/job.yaml"))
                .getConfig();

        Map<TaskId, TaskConfig> taskConfigs = Map.of(
                TaskId.of("task-id"),
                TaskConfigYamlParser.parse(TestUtils.textResource("task/validation/some-task-1.yaml"))
        );

        TaskletMetadata metadata1 = metadataFor(
                "implementation1",
                1L,
                SimpleTasklet.getDescriptor(),
                SimpleData.getDescriptor(),
                SimpleData.getDescriptor()
        ).toBuilder()
                .features(Features.empty())
                .build();

        Mockito.doReturn(metadata1).when(taskletMetadataService).fetchMetadata(
                TaskletMetadata.Id.of("implementation1", TaskletRuntime.SANDBOX, 1L)
        );

        var report = validator.validate(
                Path.of("flows/validation/a.yaml"),
                config,
                taskConfigs
        );

        assertThat(report.isValid()).isFalse();
        var outdatedTasklets = report.getViolationsByFlows().values().stream()
                .map(FlowViolations::getOutdatedTasklets)
                .toList();
        assertThat(outdatedTasklets).containsExactly(Set.of("task-id"));
    }

    @Test
    void deprecatedTasklets() throws Exception {
        AYamlConfig config = AYamlParser.parseAndValidate(TestUtils.textResource("flows/validation/job.yaml"))
                .getConfig();

        Map<TaskId, TaskConfig> taskConfigs = Map.of(
                TaskId.of("task-id"),
                TaskConfigYamlParser.parse(TestUtils.textResource("task/validation/some-deprecated-task-1.yaml"))
        );

        TaskletMetadata metadata1 = metadataFor(
                "implementation1",
                1L,
                SimpleTasklet.getDescriptor(),
                SimpleData.getDescriptor(),
                SimpleData.getDescriptor()
        ).toBuilder()
                .features(Features.empty())
                .build();

        Mockito.doReturn(metadata1).when(taskletMetadataService).fetchMetadata(
                TaskletMetadata.Id.of("implementation1", TaskletRuntime.SANDBOX, 1L)
        );

        var report = validator.validate(
                Path.of("flows/validation/a.yaml"),
                config,
                taskConfigs
        );

        assertThat(report.isValid()).isFalse();
        assertThat(report.getDeprecatedTasks()).isEqualTo(
                Map.of("task-id", "Consider migration"));
    }

    @Test
    void deprecatedTaskletsWithMessage() throws Exception {
        AYamlConfig config = AYamlParser.parseAndValidate(TestUtils.textResource("flows/validation/job.yaml"))
                .getConfig();

        Map<TaskId, TaskConfig> taskConfigs = Map.of(
                TaskId.of("task-id"),
                TaskConfigYamlParser.parse(TestUtils.textResource("task/validation/some-deprecated-task-2.yaml"))
        );

        TaskletMetadata metadata1 = metadataFor(
                "implementation1",
                1L,
                SimpleTasklet.getDescriptor(),
                SimpleData.getDescriptor(),
                SimpleData.getDescriptor()
        ).toBuilder()
                .features(Features.empty())
                .build();

        Mockito.doReturn(metadata1).when(taskletMetadataService).fetchMetadata(
                TaskletMetadata.Id.of("implementation1", TaskletRuntime.SANDBOX, 1L)
        );

        var report = validator.validate(
                Path.of("flows/validation/a.yaml"),
                config,
                taskConfigs
        );

        assertThat(report.isValid()).isFalse();
        assertThat(report.getDeprecatedTasks()).isEqualTo(
                Map.of("task-id", "Consider migration to task-1"));
    }

    @Test
    void validateJobWithInvalidMultiplyBy() throws Exception {
        var result = AYamlParser.parseAndValidate(
                TestUtils.textResource("flows/validation/job-with-invalid-multiply-by.yaml"));
        assertThat(result.isSuccess()).isFalse();
        assertThat(result.getSchemaReportMessages())
                .anyMatch(s -> s.contains("\"message\" : \"object has missing required properties ([\\\"by\\\"])\""));

        //noinspection ConstantConditions
        if (false) {
            var report = validateSingleTasklet("flows/validation/job-with-invalid-multiply-by.yaml");

            ReportId flowId = reportId("flow-with-multiply", CiProcessId.Type.FLOW);

            assertThat(report.isValid()).isFalse();
            assertThat(report.getViolationsByFlows()).containsOnlyKeys(flowId);
            assertThat(report.getViolationsByFlows().get(flowId).getViolationsByJobs())
                    .isEqualTo(Map.of("job-with-multiply", Set.of(
                            "multiply config must have \"by\" value"
                    )));
        }
    }

    @Test
    void validateJobWithInvalidTaskResourceExpression() throws Exception {
        var report = validateSingleTasklet("flows/validation/job-with-invalid-task-resource-expression.yaml");

        ReportId flowId = reportId("flow-with-invalid", CiProcessId.Type.FLOW);

        assertThat(report.isValid()).isFalse();
        assertThat(report.getViolationsByFlows()).containsOnlyKeys(flowId);
        assertThat(report.getViolationsByFlows().get(flowId).getViolationsByJobs())
                .isEqualTo(Map.of("job-with-invalid", Set.of("""
                        Unable to validate 'requirements/sandbox/tasks_resource' expression: \
                        Unable to substitute with expression "${a + b}": \
                        Unable to compile expression "a + b": syntax error token recognition error at: '+' \
                        at position 2, syntax error extraneous input 'b' expecting <EOF> at position 4"""
                )));
    }

    @Test
    void validateJobWithInvalidContainerResourceExpression() throws Exception {
        var report = validateSingleTasklet("flows/validation/job-with-invalid-container-resource-expression.yaml");

        ReportId flowId = reportId("flow-with-invalid", CiProcessId.Type.FLOW);

        assertThat(report.isValid()).isFalse();
        assertThat(report.getViolationsByFlows()).containsOnlyKeys(flowId);
        assertThat(report.getViolationsByFlows().get(flowId).getViolationsByJobs())
                .isEqualTo(Map.of("job-with-invalid", Set.of("""
                        Unable to validate 'requirements/sandbox/container_resource' expression: \
                        Unable to substitute with expression "${a + b}": \
                        Unable to compile expression "a + b": syntax error token recognition error at: '+' \
                        at position 2, syntax error extraneous input 'b' expecting <EOF> at position 4"""
                )));
    }

    @Test
    void validateJobWithInvalidRequirementsSemaphore() throws Exception {
        var report = validateSingleTasklet("flows/validation/job-with-invalid-requirements-semaphore.yaml");

        ReportId flowId = reportId("flow-with-invalid", CiProcessId.Type.FLOW);

        assertThat(report.isValid()).isFalse();
        assertThat(report.getViolationsByFlows()).containsOnlyKeys(flowId);
        assertThat(report.getViolationsByFlows().get(flowId).getViolationsByJobs())
                .isEqualTo(Map.of("job-with-invalid", Set.of("""
                        Unable to validate 'requirements/sandbox/semaphores/acquires' expression: \
                        Unable to substitute with expression "${a + b}": \
                        Unable to compile expression "a + b": syntax error token recognition error at: '+' \
                        at position 2, syntax error extraneous input 'b' expecting <EOF> at position 4"""
                )));
    }

    @Test
    void validateJobWithInvalidMultiplyByExpression() throws Exception {
        var report = validateSingleTasklet("flows/validation/job-with-invalid-multiply-by-expression.yaml");

        ReportId flowId = reportId("flow-with-multiply", CiProcessId.Type.FLOW);

        assertThat(report.isValid()).isFalse();
        assertThat(report.getViolationsByFlows()).containsOnlyKeys(flowId);
        assertThat(report.getViolationsByFlows().get(flowId).getViolationsByJobs())
                .isEqualTo(Map.of("job-with-multiply", Set.of("""
                        Unable to validate 'multiply/by' expression: Unable to substitute with expression "${a + b}": \
                        Unable to compile expression "a + b": syntax error token recognition error at: '+' \
                        at position 2, syntax error extraneous input 'b' expecting <EOF> at position 4"""
                )));
    }

    @Test
    void validateJobWithInvalidIfExpression() throws Exception {
        var report = validateSingleTasklet("flows/validation/job-with-invalid-if-expression.yaml");

        ReportId flowId = reportId("flow-with-static-resource", CiProcessId.Type.FLOW);

        assertThat(report.isValid()).isFalse();
        assertThat(report.getViolationsByFlows()).containsOnlyKeys(flowId);
        assertThat(report.getViolationsByFlows().get(flowId).getViolationsByJobs())
                .isEqualTo(Map.of("job-with-static-resource-2", Set.of("""
                        Unable to validate 'if' expression: Unable to substitute with expression "${a + b}": \
                        Unable to compile expression "a + b": syntax error token recognition error at: '+' \
                        at position 2, syntax error extraneous input 'b' expecting <EOF> at position 4"""
                )));
    }

    @Test
    void validateJobWithInvalidMultiplyNoField() throws Exception {
        var report = validateSingleTasklet("flows/validation/job-with-invalid-multiply-no-field.yaml");

        ReportId flowId = reportId("flow-with-multiply", CiProcessId.Type.FLOW);

        assertThat(report.isValid()).isFalse();
        assertThat(report.getViolationsByFlows()).containsOnlyKeys(flowId);
        assertThat(report.getViolationsByFlows().get(flowId).getViolationsByJobs())
                .isEqualTo(Map.of("job-with-multiply", Set.of(
                        "resources of following types are missed: ci.test.Simple, actual received resources: "
                )));
    }

    @Test
    void validateJobWithInvalidMultiplyInvalidField() throws Exception {
        var report = validateSingleTasklet("flows/validation/job-with-invalid-multiply-invalid-field.yaml");

        ReportId flowId = reportId("flow-with-multiply", CiProcessId.Type.FLOW);

        assertThat(report.isValid()).isFalse();
        assertThat(report.getViolationsByFlows()).containsOnlyKeys(flowId);
        assertThat(report.getViolationsByFlows().get(flowId).getViolationsByJobs())
                .isEqualTo(Map.of("job-with-multiply", Set.of(
                        "resources of following types are missed: ci.test.Simple, actual received resources: x1"
                )));
    }

    @Test
    void validateJobWithMultiplyCross() throws Exception {
        var report = validateSingleMultiplyByTasklet("flows/validation/job-with-multiply-cross.yaml");
        assertThat(report.isValid()).isTrue();
    }

    @Test
    void validateJobWithMultiplyCrossStageFrom() throws Exception {
        var report = validateSingleTasklet("flows/validation/job-with-multiply-cross-stage-from.yaml");
        assertThat(report.isValid()).isTrue();
    }

    @Test
    void validateJobWithMultiplyCrossStageTo() throws Exception {
        var report = validateSingleMultiplyByTasklet("flows/validation/job-with-multiply-cross-stage-to.yaml");
        assertThat(report.isValid()).isTrue();
    }

    @Test
    void validateJobWithInvalidMultiplyNumbers() throws Exception {
        var report = validateSingleTasklet("flows/validation/job-with-invalid-multiply-numbers.yaml");

        ReportId flowId = reportId("flow-with-multiply", CiProcessId.Type.FLOW);

        assertThat(report.isValid()).isFalse();
        assertThat(report.getViolationsByFlows()).containsOnlyKeys(flowId);
        assertThat(report.getViolationsByFlows().get(flowId).getViolationsByJobs())
                .isEqualTo(Map.of("deploy", Set.of(
                        "Dynamic job [deploy] will be generated and intersected with another job [deploy-1]"
                )));
    }

    @Test
    void validateJobWithDifferentInputsNoSuitableUnexpectedList() throws Exception {
        AYamlConfig config = AYamlParser.parseAndValidate(
                TestUtils.textResource("flows/validation/job-with-multiple-upstreams.yaml")
        ).getConfig();
        Map<TaskId, TaskConfig> taskConfigs = Map.of(
                TaskId.of("ci_test/some_task_1"),
                TaskConfigYamlParser.parse(TestUtils.textResource("task/validation/some-task-1.yaml")),
                TaskId.of("ci_test/some_task_2"),
                TaskConfigYamlParser.parse(TestUtils.textResource("task/validation/some-task-2.yaml")),
                TaskId.of("ci_test/some_task_3"),
                TaskConfigYamlParser.parse(TestUtils.textResource("task/validation/some-task-3.yaml"))
        );

        Mockito.doReturn(metadataFor(
                "implementation1",
                1L,
                SimpleNoInputTasklet.getDescriptor(),
                EmptyData.getDescriptor(),
                SimpleData.getDescriptor()
        )).when(taskletMetadataService).fetchMetadata(
                TaskletMetadata.Id.of("implementation1", TaskletRuntime.SANDBOX, 1L)
        );
        Mockito.doReturn(metadataFor(
                "implementation2",
                2L,
                SimpleTasklet.getDescriptor(),
                SimpleData.getDescriptor(),
                SimpleData.getDescriptor()
        )).when(taskletMetadataService).fetchMetadata(
                TaskletMetadata.Id.of("implementation2", TaskletRuntime.SANDBOX, 2L)
        );
        Mockito.doReturn(metadataFor(
                "implementation3",
                3L,
                DifferentInputsTasklet.getDescriptor(),
                DifferentInputsData.getDescriptor(),
                EmptyData.getDescriptor()
        )).when(taskletMetadataService).fetchMetadata(
                TaskletMetadata.Id.of("implementation3", TaskletRuntime.SANDBOX, 3L)
        );

        InputOutputTaskValidator.Report report = validator.validate(
                Path.of("flows/validation/a.yaml"),
                config,
                taskConfigs
        );

        ReportId flowId = reportId("flow-with-multiple-upstreams", CiProcessId.Type.FLOW);

        assertThat(report.isValid()).isFalse();
        assertThat(report.getViolationsByFlows()).containsOnlyKeys(flowId);
        assertThat(report.getViolationsByFlows().get(flowId).getViolationsByJobs())
                .isEqualTo(Map.of("job-with-input-3", Set.of(
                        "unexpected multiple resources of following types: " +
                                "ci.test.Simple (got from job-with-input-1, job-with-input-2)",
                        "resources of following types are missed: " +
                                "ci.test.Other, actual received resources: ci.test.Simple"
                )));
    }

    @Test
    void validateJobWithUnexpectedSingleStaticResource() throws Exception {
        AYamlConfig config = AYamlParser.parseAndValidate(
                TestUtils.textResource("flows/validation/job-with-static-resource.yaml")
        ).getConfig();
        Map<TaskId, TaskConfig> taskConfigs = Map.of(
                TaskId.of("ci_test/some_task_1"),
                TaskConfigYamlParser.parse(TestUtils.textResource("task/validation/some-task-1.yaml"))
        );

        TaskletMetadata metadata = metadataFor(
                "implementation1",
                1L,
                SimpleRepeatedTasklet.getDescriptor(),
                SimpleRepeatedData.getDescriptor(),
                SimpleRepeatedData.getDescriptor()
        );

        Mockito.doReturn(metadata).when(taskletMetadataService).fetchMetadata(
                TaskletMetadata.Id.of("implementation1", TaskletRuntime.SANDBOX, 1L)
        );

        InputOutputTaskValidator.Report report = validator.validate(
                Path.of("flows/validation/a.yaml"),
                config,
                taskConfigs
        );

        ReportId flowId = reportId("flow-with-static-resource", CiProcessId.Type.FLOW);

        assertThat(report.isValid()).isFalse();
        assertThat(report.getViolationsByFlows()).containsOnlyKeys(flowId);
        assertThat(report.getViolationsByFlows().get(flowId).getViolationsByJobs())
                .isEqualTo(
                        Map.of("job-with-static-resource", Set.of(
                                "Unable to parse protobuf input message ci.test.SimpleRepeatedData: " +
                                        "Expect an array but found: \"some string\""
                        )));
    }


    @Test
    void validateJobWithRepeatedDataAsJmesPath() throws Exception {
        AYamlConfig config = AYamlParser.parseAndValidate(
                TestUtils.textResource("flows/validation/job-with-static-resource-jmespath.yaml")
        ).getConfig();
        Map<TaskId, TaskConfig> taskConfigs = Map.of(
                TaskId.of("ci_test/some_task_1"),
                TaskConfigYamlParser.parse(TestUtils.textResource("task/validation/some-task-1.yaml"))
        );

        TaskletMetadata metadata = metadataFor(
                "implementation1",
                1L,
                SimpleInputListTasklet.getDescriptor(),
                SimpleListData.getDescriptor(),
                SimpleData.getDescriptor()
        );

        Mockito.doReturn(metadata).when(taskletMetadataService).fetchMetadata(
                TaskletMetadata.Id.of("implementation1", TaskletRuntime.SANDBOX, 1L)
        );

        InputOutputTaskValidator.Report report = validator.validate(
                Path.of("flows/validation/a.yaml"),
                config,
                taskConfigs
        );
        assertThat(report.isValid()).isTrue();
    }

    @Test
    void primitiveInput() throws JsonProcessingException, ProcessingException {
        InputOutputTaskValidator.Report report = validateSingleJobWithTask(metadataFor(
                "implementation1",
                1L,
                TaskletWithPrimitiveInput.getDescriptor(),
                PrimitiveInput.getDescriptor(),
                EmptyData.getDescriptor()
        ), "task/validation/some-task-1.yaml");

        ReportId flowId = reportId("single-job-flow", CiProcessId.Type.FLOW);

        assertThat(report.isValid()).isFalse();
        assertThat(report.getViolationsByFlows()).containsOnlyKeys(flowId);
        assertThat(report.getViolationsByFlows().get(flowId).getOtherViolations())
                .isEqualTo(
                        Set.of(
                                "tasklet input message ci.test.PrimitiveInput" +
                                        " cannot have primitive fields. int_field is int32"
                        ));
    }

    @Test
    void primitiveOutput() throws JsonProcessingException, ProcessingException {
        InputOutputTaskValidator.Report report = validateSingleJobWithTask(metadataFor(
                "implementation1",
                1L,
                TaskletWithPrimitiveInput.getDescriptor(),
                EmptyData.getDescriptor(),
                PrimitiveOutput.getDescriptor()
        ), "task/validation/some-task-1.yaml");

        ReportId flowId = reportId("single-job-flow", CiProcessId.Type.FLOW);

        assertThat(report.isValid()).isFalse();
        assertThat(report.getViolationsByFlows()).containsOnlyKeys(flowId);
        assertThat(report.getViolationsByFlows().get(flowId).getOtherViolations())
                .isEqualTo(
                        Set.of(
                                "tasklet output message ci.test.PrimitiveOutput" +
                                        " cannot have primitive fields. double_field is double"
                        ));
    }

    @Test
    void primitiveInputSingle() throws JsonProcessingException, ProcessingException {
        InputOutputTaskValidator.Report report = validateSingleJobWithTask(metadataFor(
                "implementation1",
                1L,
                TaskletWithPrimitiveInput.getDescriptor(),
                PrimitiveInput.getDescriptor(),
                EmptyData.getDescriptor()
        ), "task/validation/single-input-task.yaml");

        ReportId flowId = reportId("single-job-flow", CiProcessId.Type.FLOW);

        assertThat(report.getViolationsByFlows().get(flowId).getOtherViolations()).isEmpty();
    }

    @Test
    void primitiveOutputSingle() throws JsonProcessingException, ProcessingException {
        InputOutputTaskValidator.Report report = validateSingleJobWithTask(metadataFor(
                "implementation1",
                1L,
                TaskletWithPrimitiveInput.getDescriptor(),
                EmptyData.getDescriptor(),
                PrimitiveOutput.getDescriptor()
        ), "task/validation/single-input-task.yaml");

        ReportId flowId = reportId("single-job-flow", CiProcessId.Type.FLOW);

        assertThat(report.getViolationsByFlows().get(flowId).getOtherViolations()).isEmpty();
    }


    @Test
    void mapInput() throws JsonProcessingException, ProcessingException {
        InputOutputTaskValidator.Report report = validateSingleJobWithTask(metadataFor(
                "implementation1",
                1L,
                TaskletWithMapInput.getDescriptor(),
                MapInput.getDescriptor(),
                EmptyData.getDescriptor()
        ), "task/validation/some-task-1.yaml");

        ReportId flowId = reportId("single-job-flow", CiProcessId.Type.FLOW);

        assertThat(report.isValid()).isFalse();
        assertThat(report.getViolationsByFlows()).containsOnlyKeys(flowId);
        assertThat(report.getViolationsByFlows().get(flowId).getOtherViolations())
                .isEqualTo(
                        Set.of(
                                "tasklet input message ci.test.MapInput" +
                                        " cannot have map fields. values is map<>"
                        )
                );
    }

    @Test
    void mapOutput() throws JsonProcessingException, ProcessingException {
        InputOutputTaskValidator.Report report = validateSingleJobWithTask(metadataFor(
                "implementation1",
                1L,
                TaskletWithMapOutput.getDescriptor(),
                EmptyData.getDescriptor(),
                MapOutput.getDescriptor()
        ), "task/validation/some-task-1.yaml");

        ReportId flowId = reportId("single-job-flow", CiProcessId.Type.FLOW);

        assertThat(report.isValid()).isFalse();
        assertThat(report.getViolationsByFlows()).containsOnlyKeys(flowId);
        assertThat(report.getViolationsByFlows().get(flowId).getOtherViolations())
                .isEqualTo(
                        Set.of(
                                "tasklet output message ci.test.MapOutput" +
                                        " cannot have map fields. values is map<>"
                        ));
    }

    @Test
    void mapInputSingle() throws JsonProcessingException, ProcessingException {
        InputOutputTaskValidator.Report report = validateSingleJobWithTask(metadataFor(
                "implementation1",
                1L,
                TaskletWithMapInput.getDescriptor(),
                MapInput.getDescriptor(),
                EmptyData.getDescriptor()
        ), "task/validation/single-input-task.yaml");

        ReportId flowId = reportId("single-job-flow", CiProcessId.Type.FLOW);

        assertThat(report.getViolationsByFlows().get(flowId).getOtherViolations()).isEmpty();
    }

    @Test
    void mapOutputSingle() throws JsonProcessingException, ProcessingException {
        InputOutputTaskValidator.Report report = validateSingleJobWithTask(metadataFor(
                "implementation1",
                1L,
                TaskletWithMapOutput.getDescriptor(),
                EmptyData.getDescriptor(),
                MapOutput.getDescriptor()
        ), "task/validation/single-input-task.yaml");

        ReportId flowId = reportId("single-job-flow", CiProcessId.Type.FLOW);

        assertThat(report.getViolationsByFlows().get(flowId).getOtherViolations()).isEmpty();
    }

    @Test
    void mapInputMap() throws JsonProcessingException, ProcessingException {
        InputOutputTaskValidator.Report report = validateSingleJobWithTask(metadataFor(
                "implementation1",
                1L,
                TaskletWithMapContainer.getDescriptor(),
                MapContainer.getDescriptor(),
                EmptyData.getDescriptor()
                ),
                "task/validation/some-task-1.yaml",
                "flows/validation/job-with-map-arguments.yaml");

        assertThat(report.isValid()).isTrue();
    }

    @Test
    void mapInputPrimitiveMap() throws JsonProcessingException, ProcessingException {
        InputOutputTaskValidator.Report report = validateSingleJobWithTask(metadataFor(
                "implementation1",
                1L,
                TaskletWithPrimitiveMapContainer.getDescriptor(),
                PrimitiveMapContainer.getDescriptor(),
                EmptyData.getDescriptor()
                ),
                "task/validation/some-task-1.yaml",
                "flows/validation/job-with-primitive-map-arguments.yaml");

        assertThat(report.isValid()).isTrue();
    }

    @Test
    void mapOneOfTasklet() throws JsonProcessingException, ProcessingException {
        InputOutputTaskValidator.Report report = validateSingleJobWithTask(metadataFor(
                "implementation1",
                1L,
                OneOfTasklet.getDescriptor(),
                OneOfInput.getDescriptor(),
                EmptyData.getDescriptor()
                ),
                "task/validation/some-task-1.yaml",
                "flows/validation/job-with-one-of-arguments.yaml");

        assertThat(report.isValid()).isTrue();
    }

    @Test
    void missedSchemaAttributes() throws JsonProcessingException, ProcessingException {
        AYamlConfig config = AYamlParser.parseAndValidate(
                TestUtils.textResource("flows/validation/job.yaml")
        ).getConfig();
        Map<TaskId, TaskConfig> taskConfigs = Map.of(
                TaskId.of("task-id"),
                TaskConfigYamlParser.parse(TestUtils.textResource("task/validation/some-task-1.yaml"))
        );

        TaskletMetadataValidationException fetchMetadataException = new TaskletMetadataValidationException(
                "fields not found"
        );

        Mockito.doThrow(fetchMetadataException).when(taskletMetadataService).fetchMetadata(
                TaskletMetadata.Id.of("implementation1", TaskletRuntime.SANDBOX, 1L)
        );

        InputOutputTaskValidator.Report report = validator.validate(
                Path.of("flows/validation/a.yaml"),
                config,
                taskConfigs
        );

        ReportId flowId = reportId("single-job-flow", CiProcessId.Type.FLOW);

        assertThat(report.isValid()).isFalse();
        assertThat(report.getViolationsByFlows()).containsOnlyKeys(flowId);
        assertThat(report.getViolationsByFlows().get(flowId).getViolationsByJobs())
                .isEqualTo(Map.of("job", Set.of(
                        "fields not found"
                )));
    }

    //


    @Test
    void validateSandboxJob() throws Exception {
        var report = validateSandboxTask("flows/validation/sandbox-job.yaml");
        assertThat(report.isValid()).isTrue();
    }

    @Test
    void validateSandboxWithUpstreamJob() throws Exception {
        var report = validateSandboxTask("flows/validation/sandbox-job-with-upstream.yaml");
        assertThat(report.isValid()).isTrue();
    }

    @Test
    void validateSandboxWithParameterizedUpstreamJob() throws Exception {
        var report = validateSandboxTask("flows/validation/sandbox-job-with-parameterized-upstream.yaml");
        assertThat(report.isValid()).isTrue();
    }

    @Test
    void validateSandboxWithParameterizedUpstreamTaskletJob() throws Exception {
        prepareDefaultTasklets();

        var report = validateSandboxTask("flows/validation/sandbox-job-with-parameterized-upstream-tasklet.yaml");
        assertThat(report.isValid()).isTrue();
    }

    @Test
    void validateSandboxWithUnknownDependencyJob() throws Exception {
        var result = AYamlParser.parseAndValidate(
                TestUtils.textResource("flows/validation/sandbox-job-unknown-task-dependency.yaml"));
        assertThat(result.isSuccess()).isFalse();
        assertThat(result.getStaticErrors()).isEqualTo(Set.of(
                "Job 'job-1' in /ci/flows/flow-invalid-job/jobs/job-with-input-2/needs not found " +
                        "in /ci/flows/flow-invalid-job/jobs",
                "Job 'job-2' in /ci/flows/flow-no-job/jobs/job-with-input-2/needs not found " +
                        "in /ci/flows/flow-no-job/jobs",
                "Job 'job-1t' in /ci/flows/flow-invalid-job-tasklet/jobs/job-with-input-2t/needs not found " +
                        "in /ci/flows/flow-invalid-job-tasklet/jobs",
                "Job 'job-2t' in /ci/flows/flow-no-job-tasklet/jobs/job-with-input-2t/needs not found " +
                        "in /ci/flows/flow-no-job-tasklet/jobs"
        ));

        //noinspection ConstantConditions
        if (false) {
            prepareDefaultTasklets();

            var report = validateSandboxTask("flows/validation/sandbox-job-unknown-task-dependency.yaml");
            assertThat(report.isValid()).isFalse();

            // Пока что проверяем только зависимости на неверные "needs"

            var invalidJobFlow = reportId("flow-invalid-job", CiProcessId.Type.FLOW);
            var noJobFlow = reportId("flow-no-job", CiProcessId.Type.FLOW);

            var invalidJobTaskletFlow = reportId("flow-invalid-job-tasklet", CiProcessId.Type.FLOW);
            var noJobTaskletFlow = reportId("flow-no-job-tasklet", CiProcessId.Type.FLOW);

            assertThat(report.isValid()).isFalse();
            assertThat(report.getViolationsByFlows())
                    .containsOnlyKeys(invalidJobFlow, noJobFlow, invalidJobTaskletFlow, noJobTaskletFlow);

            assertThat(report.getViolationsByFlows().get(invalidJobFlow).getViolationsByJobs())
                    .isEqualTo(Map.of("job-with-input-2", Set.of(
                            "Job with name \"job-1\" is not found"
                    )));

            assertThat(report.getViolationsByFlows().get(noJobFlow).getViolationsByJobs())
                    .isEqualTo(Map.of("job-with-input-2", Set.of(
                            "Job with name \"job-2\" is not found"
                    )));

            assertThat(report.getViolationsByFlows().get(invalidJobTaskletFlow).getViolationsByJobs())
                    .isEqualTo(Map.of("job-with-input-2t", Set.of(
                            "Job with name \"job-1t\" is not found"
                    )));

            assertThat(report.getViolationsByFlows().get(noJobTaskletFlow).getViolationsByJobs())
                    .isEqualTo(Map.of("job-with-input-2t", Set.of(
                            "Job with name \"job-2t\" is not found"
                    )));
        }
    }

    @Test
    void validateSandboxWithInvalidMultiply() throws Exception {
        prepareDefaultTasklets();

        var report = validateSandboxTask("flows/validation/sandbox-job-with-invalid-multiply.yaml");
        assertThat(report.isValid()).isFalse();

        // We does not support multiple inputs merge so far
        var unsupportedFlow = reportId("flow-with-multiply", CiProcessId.Type.FLOW);
        assertThat(report.getViolationsByFlows().get(unsupportedFlow).getViolationsByJobs())
                .isEqualTo(Map.of("job-with-multiply", Set.of(
                        Type.SANDBOX_TASK + " multiply config cannot have \"as-field\" value"
                )));
    }

    @Test
    void validateWithFlowVars() throws Exception {
        var report = validateSandboxTask("flows/validation/job-with-flow-vars.yaml");
        assertThat(report.isValid()).isTrue();
    }

    @Test
    void validateWithInvalidFlowVars() throws Exception {
        var report = validateSandboxTask("flows/validation/job-with-invalid-flow-vars.yaml");
        assertThat(report.isValid()).isFalse();

        var invalidFlowInRelease = reportId("sample", CiProcessId.Type.RELEASE)
                .withFlowReference(FlowReference.of("flow-with-vars", Common.FlowType.FT_DEFAULT));

        assertThat(report.getViolationsByFlows().get(invalidFlowInRelease).getViolationsByJobs())
                .isEqualTo(Map.of("job-with-vars", Set.of(
                        "Unable to substitute with expression \"${flow-vars.var2}\": " +
                                "No value resolved for expression: flow-vars.var2"
                )));
        assertThat(invalidFlowInRelease.toString())
                .isEqualTo("Release with id \"sample\"");

        var invalidFlowInAction = reportId("sample-action", CiProcessId.Type.FLOW)
                .withFlowReference(FlowReference.of("flow-with-vars", Common.FlowType.FT_DEFAULT))
                .withSubType("Action");
        assertThat(report.getViolationsByFlows().get(invalidFlowInAction).getViolationsByJobs())
                .isEqualTo(Map.of("job-with-vars", Set.of(
                        "Unable to substitute with expression \"${flow-vars.var2}\": " +
                                "No value resolved for expression: flow-vars.var2"
                )));
        assertThat(invalidFlowInAction.toString())
                .isEqualTo("Action with id \"sample-action\"");
    }

    private ReportId reportId(String sample, CiProcessId.Type release) {
        return ReportId.of(sample, release)
                .withFlowReference(FlowReference.of(sample, Common.FlowType.FT_DEFAULT));
    }

    @Test
    void validateWithInvalidFlowVarsInManual() throws Exception {
        var report = validateSandboxTask("flows/validation/job-with-invalid-flow-vars-manual.yaml");
        assertThat(report.isValid()).isFalse();

        var invalidFlowInRelease = reportId("sample", CiProcessId.Type.RELEASE)
                .withFlowReference(FlowReference.of("flow-with-vars", Common.FlowType.FT_DEFAULT));

        assertThat(report.getViolationsByFlows().get(invalidFlowInRelease).getViolationsByJobs())
                .isEqualTo(Map.of("job-with-vars", Set.of(
                        "Unable to substitute with expression \"Access ${flow-vars.var2}?\": " +
                                "No value resolved for expression: flow-vars.var2"
                )));
        assertThat(invalidFlowInRelease.toString())
                .isEqualTo("Release with id \"sample\"");

        var invalidFlowInAction = reportId("sample-action", CiProcessId.Type.FLOW)
                .withFlowReference(FlowReference.of("flow-with-vars", Common.FlowType.FT_DEFAULT))
                .withSubType("Action");
        assertThat(report.getViolationsByFlows().get(invalidFlowInAction).getViolationsByJobs())
                .isEqualTo(Map.of("job-with-vars", Set.of(
                        "Unable to substitute with expression \"Access ${flow-vars.var2}?\": " +
                                "No value resolved for expression: flow-vars.var2"
                )));
        assertThat(invalidFlowInAction.toString())
                .isEqualTo("Action with id \"sample-action\"");
    }

    @Test
    void validateHotfixWithInvalidFlowVars() throws Exception {
        var report = validateSandboxTask("flows/validation/hotfix-flow-with-invalid-flow-vars.yaml");
        assertThat(report.isValid()).isFalse();

        assertThat(report.getViolationsByFlows()).isEqualTo(
                Map.of(
                        new ReportId("sample", CiProcessId.Type.RELEASE,
                                FlowReference.of("flow-with-vars2", Common.FlowType.FT_HOTFIX), null),
                        FlowViolations.builder()
                                .violationsByJob("job-with-multiply", Set.of(
                                        "Unable to substitute with expression \"${flow-vars.var2}\": " +
                                                "No value resolved for expression: flow-vars.var2"))
                                .build(),
                        new ReportId("sample", CiProcessId.Type.RELEASE,
                                FlowReference.of("flow-with-vars2", Common.FlowType.FT_ROLLBACK), null),
                        FlowViolations.builder()
                                .violationsByJob("job-with-multiply", Set.of(
                                        "Unable to substitute with expression \"${flow-vars.var2}\": " +
                                                "No value resolved for expression: flow-vars.var2"))
                                .build()
                )
        );
    }

    @Test
    void validateHotfixWithInvalidFlowVarsReverse() throws Exception {
        var report = validateSandboxTask("flows/validation/hotfix-flow-with-invalid-flow-vars-reverse.yaml");
        assertThat(report.isValid()).isFalse();

        assertThat(report.getViolationsByFlows()).isEqualTo(
                Map.of(
                        reportId("sample", CiProcessId.Type.RELEASE)
                                .withFlowReference(FlowReference.of("flow-with-vars", Common.FlowType.FT_DEFAULT)),
                        FlowViolations.builder()
                                .violationsByJob("job-with-multiply", Set.of(
                                        "Unable to substitute with expression \"${flow-vars.var1}\": " +
                                                "No value resolved for expression: flow-vars.var1"))
                                .build(),
                        new ReportId("sample", CiProcessId.Type.RELEASE,
                                FlowReference.of("flow-with-vars2", Common.FlowType.FT_HOTFIX), null),
                        FlowViolations.builder()
                                .violationsByJob("job-with-multiply", Set.of(
                                        "Unable to substitute with expression \"${flow-vars.var2}\": " +
                                                "No value resolved for expression: flow-vars.var2"))
                                .build(),
                        new ReportId("sample", CiProcessId.Type.RELEASE,
                                FlowReference.of("flow-with-vars2", Common.FlowType.FT_ROLLBACK), null),
                        FlowViolations.builder()
                                .violationsByJob("job-with-multiply", Set.of(
                                        "Unable to substitute with expression \"${flow-vars.var2}\": " +
                                                "No value resolved for expression: flow-vars.var2"))
                                .build()
                )
        );
    }

    @Test
    void validateHotfixWithMultipleStages() throws Exception {
        var report = validateSandboxTask("flows/validation/hotfix-flow-with-multiple-stages.yaml");
        assertThat(report.isValid()).isTrue();
    }

    @Test
    void validateFlowWithFlowVars() throws Exception {
        var report = validateSingleTasklet("flows/validation/job-with-flow-vars-in-version.yaml");
        assertThat(report.isValid()).isTrue();
    }

    @Test
    void validateFlowWithFlowVarsUiCannotMakeFakeData() throws Exception {
        var report = validateSingleTasklet("flows/validation/flow-with-complex-flow-vars.yaml");
        assertThat(report.isValid()).isTrue();
    }

    @Test
    void validateFlowWithInvalidFlowVars() throws Exception {
        var report = validateSingleTasklet("flows/validation/job-with-invalid-flow-vars-in-version.yaml");

        ReportId flowId = reportId("sample", CiProcessId.Type.RELEASE)
                .withFlowReference(FlowReference.of("flow-with-vars", Common.FlowType.FT_DEFAULT));

        assertThat(report.isValid()).isFalse();
        assertThat(report.getViolationsByFlows()).containsOnlyKeys(flowId);
        assertThat(report.getViolationsByFlows().get(flowId).getViolationsByJobs())
                .isEqualTo(Map.of("job-with-vars", Set.of("""
                        not found version 'unknown' for job 'job-with-vars (job-with-vars)'. \
                        Available versions: stable"""
                )));
    }

    private InputOutputTaskValidator.Report validateSingleJobWithTask(TaskletMetadata metadata, String taskPath)
            throws JsonProcessingException, ProcessingException {
        return validateSingleJobWithTask(metadata, taskPath, "flows/validation/job.yaml");
    }

    private InputOutputTaskValidator.Report validateSingleJobWithTask(TaskletMetadata metadata,
                                                                      String taskPath,
                                                                      String yaml)
            throws JsonProcessingException, ProcessingException {

        AYamlConfig config = AYamlParser.parseAndValidate(TestUtils.textResource(yaml)).getConfig();
        Map<TaskId, TaskConfig> taskConfigs = Map.of(
                TaskId.of("task-id"),
                TaskConfigYamlParser.parse(TestUtils.textResource(taskPath))
        );

        Mockito.doReturn(metadata).when(taskletMetadataService).fetchMetadata(
                TaskletMetadata.Id.of("implementation1", TaskletRuntime.SANDBOX, 1L)
        );

        return validator.validate(
                Path.of("flows/validation/a.yaml"),
                config,
                taskConfigs
        );
    }

    private void prepareDefaultTasklets() {
        Mockito.doReturn(metadataFor(
                "implementation1",
                1L,
                SimpleNoInputTasklet.getDescriptor(),
                EmptyData.getDescriptor(),
                SimpleData.getDescriptor()
        )).when(taskletMetadataService).fetchMetadata(
                TaskletMetadata.Id.of("implementation1", TaskletRuntime.SANDBOX, 1L)
        );
        Mockito.doReturn(metadataFor(
                "implementation2",
                2L,
                SimpleTasklet.getDescriptor(),
                SimpleData.getDescriptor(),
                SimpleData.getDescriptor()
        )).when(taskletMetadataService).fetchMetadata(
                TaskletMetadata.Id.of("implementation2", TaskletRuntime.SANDBOX, 2L)
        );
        Mockito.doReturn(metadataFor(
                "implementation3",
                3L,
                SimpleNoOutputTasklet.getDescriptor(),
                SimpleData.getDescriptor(),
                EmptyData.getDescriptor()
        )).when(taskletMetadataService).fetchMetadata(
                TaskletMetadata.Id.of("implementation3", TaskletRuntime.SANDBOX, 3L)
        );
    }

    private InputOutputTaskValidator.Report validateSandboxTask(String aYaml) throws Exception {
        AYamlConfig config = AYamlParser.parseAndValidate(TestUtils.textResource(aYaml)).getConfig();
        return validator.validate(
                Path.of("flows/validation/a.yaml"),
                config,
                sandboxTaskConfig());
    }

    private InputOutputTaskValidator.Report validateFlowWith2Tasks(
            String aYaml,
            TaskletMetadata metadata1,
            TaskletMetadata metadata2) throws Exception {
        AYamlConfig config = AYamlParser.parseAndValidate(TestUtils.textResource(aYaml)).getConfig();
        Map<TaskId, TaskConfig> taskConfigs = Map.of(
                TaskId.of("ci_test/some_task_1"),
                TaskConfigYamlParser.parse(TestUtils.textResource("task/validation/some-task-1.yaml")),
                TaskId.of("ci_test/some_task_2"),
                TaskConfigYamlParser.parse(TestUtils.textResource("task/validation/some-task-2.yaml"))
        );


        Mockito.doReturn(metadata1)
                .when(taskletMetadataService)
                .fetchMetadata(TaskletMetadata.Id.of("implementation1", TaskletRuntime.SANDBOX, 1L));

        Mockito.doReturn(metadata2)
                .when(taskletMetadataService)
                .fetchMetadata(TaskletMetadata.Id.of("implementation2", TaskletRuntime.SANDBOX, 2L));

        return validator.validate(
                Path.of("flows/validation/a.yaml"),
                config,
                taskConfigs
        );
    }

    private InputOutputTaskValidator.Report validateSingleTasklet(String aYaml) throws Exception {
        var metadata1 = metadataFor(
                "implementation1",
                1L,
                SimpleTasklet.getDescriptor(),
                SimpleData.getDescriptor(),
                SimpleData.getDescriptor()
        );
        var metadata2 = metadataFor(
                "implementation2",
                1L,
                TaskletWithPrimitiveInputData.getDescriptor(),
                PrimitiveInputData.getDescriptor(),
                EmptyData.getDescriptor()
        );
        return validateFlowWith2Tasks(aYaml, metadata1, metadata2);
    }

    private InputOutputTaskValidator.Report validateSingleMultiplyByTasklet(String aYaml) throws Exception {
        var metadata1 = metadataFor(
                "implementation1",
                1L,
                SimpleTasklet.getDescriptor(),
                SimpleData.getDescriptor(),
                SimpleData.getDescriptor()
        );
        var metadata2 = metadataFor(
                "implementation2",
                2L,
                SimpleInputListTasklet.getDescriptor(),
                SimpleListData.getDescriptor(),
                SimpleData.getDescriptor()
        );
        return validateFlowWith2Tasks(aYaml, metadata1, metadata2);
    }

    private static Map<TaskId, TaskConfig> sandboxTaskConfig() {
        var sandboxTasks = initConfig(4, "ci-test/some-sandbox-task", "task/validation/some-sandbox-task");
        var taskletTasks = initConfig(3, "ci-test/some-task", "task/validation/some-task");
        sandboxTasks.putAll(taskletTasks);
        return sandboxTasks;
    }

    private static TaskConfig parse(String resource) {
        try {
            return TaskConfigYamlParser.parse(resource);
        } catch (JsonProcessingException e) {
            throw new RuntimeException(e);
        }
    }

    private static Map<TaskId, TaskConfig> initConfig(int to, String taskPrefix, String resourcePrefix) {
        return IntStream.rangeClosed(1, to)
                .boxed()
                .collect(Collectors.toMap(
                        id -> TaskId.of(taskPrefix + "-" + id),
                        id -> parse(TestUtils.textResource(resourcePrefix + "-" + id + ".yaml"))
                ));
    }

}
