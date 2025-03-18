package ru.yandex.ci.tms.client;

import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.function.Supplier;
import java.util.stream.Collectors;

import com.google.common.collect.Streams;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.Arguments;
import org.junit.jupiter.params.provider.MethodSource;
import org.mockito.Mock;
import org.mockserver.integration.ClientAndServer;
import org.mockserver.model.Headers;
import org.mockserver.model.HttpRequest;
import org.mockserver.model.HttpResponse;
import org.mockserver.model.HttpStatusCode;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpMethod;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.CommonTestBase;
import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.client.sandbox.api.ResourceState;
import ru.yandex.ci.client.sandbox.api.SandboxTaskRequirements;
import ru.yandex.ci.client.sandbox.api.SandboxTaskStatus;
import ru.yandex.ci.core.config.registry.SandboxTaskBadgesConfig;
import ru.yandex.ci.core.job.JobResource;
import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.core.tasklet.Features;
import ru.yandex.ci.engine.SandboxTaskService;
import ru.yandex.ci.engine.flow.SandboxTestData;
import ru.yandex.ci.engine.flow.TaskBadgeService;
import ru.yandex.ci.flow.engine.runtime.state.JobProgressService;
import ru.yandex.ci.flow.engine.runtime.state.model.TaskBadge;
import ru.yandex.ci.tasklet.SandboxResource;
import ru.yandex.ci.test.TestUtils;
import ru.yandex.ci.tms.spring.clients.SandboxClientTestConfig;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.Mockito.mock;
import static org.mockserver.model.Header.header;
import static org.mockserver.model.NottableString.not;
import static org.mockserver.model.NottableString.string;

@ContextConfiguration(classes = {
        SandboxClientTestConfig.class
})
public class SandboxTaskServiceTest extends CommonTestBase {
    private static final long TASK_ID = 12345;
    private static final int YA_PACKAGE_TASK_ID = 681117898;

    private static final Headers QUOTA_HEADERS = new Headers(
            header("X-Api-Quota-Milliseconds", 1007),
            header("X-Api-Consumption-Milliseconds", 103)
    );

    @Autowired
    public ClientAndServer sandboxServer;

    @Autowired
    private SandboxClient sandboxClient;

    private SandboxTaskService sandboxTaskService;

    private TaskBadgeService taskBadgeService;

    @Mock
    private JobProgressService jobProgressService;

    @BeforeEach
    public void setUp() {
        sandboxTaskService = new SandboxTaskService(sandboxClient, "test", null);
        taskBadgeService = new TaskBadgeService(jobProgressService);
        sandboxServer.reset();
    }

    @Test
    public void createTest() {
        sandboxServer.when(HttpRequest.request("/v1.0/task")
                .withMethod(HttpMethod.POST.name()))
                .respond(SandboxTestData.responseFromResource("new_task.json"));

        long newTaskId = sandboxTaskService.createTaskletTask(
                "TASKLET_SUM",
                "SumPy",
                "Task for tasklet SumPy",
                new JsonObject(),
                List.of(),
                List.of(),
                Map.of(),
                SandboxTaskService.RuntimeSettings.builder()
                        .requirements(new SandboxTaskRequirements().setTasksResource(1234L))
                        .taskletFeatures(Features.empty())
                        .build()
        );

        assertThat(newTaskId)
                .isEqualTo(TASK_ID);
    }

    @Test
    public void getStatusTest() {

        sandboxServer.when(HttpRequest.request("/v1.0/task")
                .withQueryStringParameter("limit", String.valueOf(1))
                .withQueryStringParameter("fields", "status")
                .withQueryStringParameter("id", String.valueOf(TASK_ID))
                .withMethod(HttpMethod.GET.name()))
                .respond(SandboxTestData.responseFromResource("created_task_batch_status.json"));

        assertThat(sandboxTaskService.getStatus(TASK_ID))
                .isEqualTo(SandboxTaskStatus.SUCCESS);
    }

    @Test
    public void getStatusMissedOnSecondary() {
        Supplier<HttpRequest> requestBuilder = () -> HttpRequest.request("/v1.0/task")
                .withQueryStringParameter("limit", String.valueOf(1))
                .withQueryStringParameter("fields", "status")
                .withQueryStringParameter("id", String.valueOf(TASK_ID))
                .withMethod(HttpMethod.GET.name());

        var emptyResponse = """
                {
                    "items": [ ],
                    "total": 0,
                    "limit": 1,
                    "offset": 0
                }
                """;

        sandboxServer.when(requestBuilder.get().withHeader(not("X-Read-Preference"), string(".*")))
                .respond(HttpResponse.response(emptyResponse).withHeaders(QUOTA_HEADERS));
        sandboxServer.when(requestBuilder.get().withHeader("X-Read-Preference", "PRIMARY"))
                .respond(SandboxTestData.responseFromResource("created_task_batch_status.json"));

        assertThat(sandboxTaskService.getStatus(TASK_ID))
                .isEqualTo(SandboxTaskStatus.SUCCESS);
    }

    @Test
    public void getStatusUnknownTest() {
        sandboxServer.when(HttpRequest.request("/v1.0/task")
                .withQueryStringParameter("limit", String.valueOf(1))
                .withQueryStringParameter("fields", "status")
                .withQueryStringParameter("id", String.valueOf(TASK_ID))
                .withMethod(HttpMethod.GET.name()))
                .respond(SandboxTestData.responseFromResource("created_task_batch_status_with_unknown_status.json"));

        assertThat(sandboxTaskService.getStatus(TASK_ID))
                .isEqualTo(SandboxTaskStatus.UNKNOWN);
    }

    @Test
    public void getOutputTest() {
        sandboxServer.when(HttpRequest.request("/v1.0/task/" + TASK_ID)
                .withMethod(HttpMethod.GET.name()))
                .respond(SandboxTestData.responseFromResource("created_task.json"));

        JsonObject expected = new JsonObject();
        expected.addProperty("sumValue", 3);
        expected.addProperty("@type", "type.googleapis.com/SimpleExample.Output");

        var taskOutput = sandboxTaskService.getSandboxClient().getTask(TASK_ID);

        var actual = sandboxTaskService.toTaskletResult(
                taskOutput.getData(),
                mock(TaskBadgeService.class),
                List.of(),
                "",
                false
        ).getOutput();

        assertThat(actual).isEqualTo(expected);
    }

    @Test
    public void getOutputSandboxTest() {
        sandboxServer.when(HttpRequest.request("/v1.0/task/" + YA_PACKAGE_TASK_ID)
                .withMethod(HttpMethod.GET.name()))
                .respond(SandboxTestData.responseFromResource("ya_package_task.json"));

        var taskOutput = sandboxTaskService.getSandboxClient().getTask(YA_PACKAGE_TASK_ID);
        var actual = sandboxTaskService.toTaskletResult(
                taskOutput.getData(),
                mock(TaskBadgeService.class),
                List.of(),
                "",
                false
        ).getOutput();


        assertThat(actual).isNotNull();
    }

    @ParameterizedTest
    @MethodSource
    public void getResources(String resource, String expect, List<ResourceState> ignoreResourceStates) {
        sandboxServer.when(HttpRequest.request("/v2/resource")
                .withQueryStringParameter("task_id", String.valueOf(YA_PACKAGE_TASK_ID))
                .withMethod(HttpMethod.GET.name()))
                .respond(SandboxTestData.responseFromResource(resource)
                        .withStatusCode(HttpStatusCode.OK_200.code()));

        List<JobResource> resources = sandboxTaskService.getTaskResources(YA_PACKAGE_TASK_ID,
                Set.copyOf(ignoreResourceStates));

        assertThat(resources).extracting(JobResource::getResourceType)
                .containsOnly(JobResourceType.ofDescriptor(SandboxResource.getDescriptor()));

        assertThat(resources).extracting(JobResource::getData).doesNotContainNull();

        List<JsonObject> expected =
                Streams.stream(TestUtils.parseGson(expect).getAsJsonArray())
                        .map(JsonElement::getAsJsonObject)
                        .collect(Collectors.toList());

        assertThat(resources)
                .extracting(JobResource::getData)
                .isEqualTo(expected);
    }

    @Test
    public void getAdditionalTaskReportStatesTest() {
        sandboxServer.when(HttpRequest.request("/v1.0/task/" + TASK_ID)
                .withMethod(HttpMethod.GET.name()))
                .respond(SandboxTestData.responseFromResource("task_with_report_states.json")
                        .withStatusCode(HttpStatusCode.OK_200.code()));
        List<TaskBadge> expected = List.of(
                TaskBadge.of(
                        "my_report_id", "SAMOGON", "http://some.url.ru/",
                        TaskBadge.TaskStatus.SUCCESSFUL, 1.0f, null, false
                )
        );

        var taskOutput = sandboxClient.getTask(TASK_ID).getData();
        var result = sandboxTaskService.toTaskletResult(taskOutput, taskBadgeService, List.of(), "", false);
        assertThat(result.getTaskBadges()).isEqualTo(expected);
    }

    @Test
    public void getPredefinedTaskReportStatesTest() {
        sandboxServer.when(HttpRequest.request("/v1.0/task/" + TASK_ID)
                .withMethod(HttpMethod.GET.name()))
                .respond(SandboxTestData.responseFromResource("task_with_predefined_report_states.json")
                        .withStatusCode(HttpStatusCode.OK_200.code()));
        var expected = List.of(
                TaskBadge.of(
                        "my_report_id-collision", "SAMOGON", "http://some.url.ru/",
                        TaskBadge.TaskStatus.SUCCESSFUL, 1.0f, null, false
                ),
                TaskBadge.of(
                        "my_report_id", "SAMOGON", "http://some.url.ru/",
                        TaskBadge.TaskStatus.SUCCESSFUL, 1.0f, null, false
                ));
        var badges = List.of(
                SandboxTaskBadgesConfig.of("my_report_id", "SAMOGON"),
                SandboxTaskBadgesConfig.of("my_report_id-with_incorrect_data", "SAMOGON"),
                SandboxTaskBadgesConfig.of("my_report_id-not_object", "SAMOGON")
        );

        var taskOutput = sandboxClient.getTask(TASK_ID).getData();
        var result = sandboxTaskService.toTaskletResult(taskOutput, taskBadgeService, badges, "", false);
        assertThat(result.getTaskBadges()).isEqualTo(expected);
    }

    @Test
    public void getPredefinedCollidedTaskReportStatesTest() {
        sandboxServer.when(HttpRequest.request("/v1.0/task/" + TASK_ID)
                .withMethod(HttpMethod.GET.name()))
                .respond(SandboxTestData.responseFromResource("task_with_predefined_report_states.json")
                        .withStatusCode(HttpStatusCode.OK_200.code()));
        var expected = List.of(
                TaskBadge.of(
                        "my_report_id-collision", "TEAMCITY", "http://other.url.ru/",
                        TaskBadge.TaskStatus.SUCCESSFUL, 1.0f, null, false
                )
        );
        var badges = List.of(SandboxTaskBadgesConfig.of("my_report_id-collision", "TEAMCITY"));

        var taskOutput = sandboxClient.getTask(TASK_ID).getData();
        var result = sandboxTaskService.toTaskletResult(taskOutput, taskBadgeService, badges, "", false);
        assertThat(result.getTaskBadges()).isEqualTo(expected);
    }

    @Test
    public void getTaskBadgesFromReportsTest() {
        sandboxServer.when(HttpRequest.request("/v1.0/task/" + TASK_ID)
                .withMethod(HttpMethod.GET.name()))
                .respond(SandboxTestData.responseFromResource("task_with_reports.json"));

        var badges = List.of(
                SandboxTaskBadgesConfig.of("build_report", "SAMOGON"),
                SandboxTaskBadgesConfig.of("collided_id")
        );


        var taskOutput = sandboxClient.getTask(TASK_ID).getData();
        var url = "http://some.task.ru/task_id";
        var result = sandboxTaskService.toTaskletResult(taskOutput, taskBadgeService, badges, url, false);
        assertThat(result.isSuccess()).isTrue();

        var expectOutput = JsonParser.parseString("""
                {
                  "__tasklet_output__": {
                    "sumValue": 3,
                    "@type": "type.googleapis.com/SimpleExample.Output"
                  },
                  "tasklet_result": "{\\n    \\"output\\": {},\\n    \\"success\\": true\\n}",
                  "_ci:task-badge:collided_id": {
                    "module": "SAMOGON",
                    "url": "http://some.url.ru/",
                    "status": "SUCCESSFUL",
                    "progress": 1.0
                  }
                }
                """);
        assertThat(result.getOutput()).isEqualTo(expectOutput);

        var expected = List.of(
                TaskBadge.of(
                        "build_report", "SAMOGON", "http://some.task.ru/task_id/build_report",
                        TaskBadge.TaskStatus.SUCCESSFUL, null, null, false
                ),
                TaskBadge.of(
                        "collided_id", "SAMOGON", "http://some.url.ru/",
                        TaskBadge.TaskStatus.SUCCESSFUL, 1.0f, null, false
                )
        );
        assertThat(result.getTaskBadges()).isEqualTo(expected);
    }

    @Test
    public void getTaskletBadgesFromReportsTest() {
        sandboxServer.when(HttpRequest.request("/v1.0/task/" + TASK_ID)
                .withMethod(HttpMethod.GET.name()))
                .respond(SandboxTestData.responseFromResource("tasklet_with_reports.json"));

        var badges = List.of(
                SandboxTaskBadgesConfig.of("build_report", "SAMOGON"),
                SandboxTaskBadgesConfig.of("collided_id")
        );


        var taskOutput = sandboxClient.getTask(TASK_ID).getData();
        var url = "http://some.task.ru/task_id";
        var result = sandboxTaskService.toTaskletResult(taskOutput, taskBadgeService, badges, url, false);
        assertThat(result.isSuccess()).isTrue();

        var expectOutput = JsonParser.parseString("""
                {
                   "sumValue": 3,
                   "@type": "type.googleapis.com/SimpleExample.Output"
                }
                """);
        assertThat(result.getOutput()).isEqualTo(expectOutput);

        var expected = List.of(
                TaskBadge.of(
                        "build_report", "SAMOGON", "http://some.task.ru/task_id/build_report",
                        TaskBadge.TaskStatus.SUCCESSFUL, null, null, false
                ),
                TaskBadge.of(
                        "collided_id", "SAMOGON", "http://some.url.ru/",
                        TaskBadge.TaskStatus.SUCCESSFUL, 1.0f, null, false
                )
        );
        assertThat(result.getTaskBadges()).isEqualTo(expected);
    }

    @Test
    public void getTaskletBadgesFromReportsFailureDefaultTest() {
        sandboxServer.when(HttpRequest.request("/v1.0/task/" + TASK_ID)
                .withMethod(HttpMethod.GET.name()))
                .respond(SandboxTestData.responseFromResource("tasklet_with_reports_failure.json"));

        var badges = List.of(
                SandboxTaskBadgesConfig.of("build_report", "SAMOGON"),
                SandboxTaskBadgesConfig.of("collided_id")
        );

        var taskOutput = sandboxClient.getTask(TASK_ID).getData();
        var url = "http://some.task.ru/task_id";
        var result = sandboxTaskService.toTaskletResult(taskOutput, taskBadgeService, badges, url, false);
        assertThat(result.isSuccess()).isFalse();
        assertThat(result.getOutput()).isNull();
        assertThat(result.getTaskBadges()).isNull();
    }

    @Test
    public void getTaskletBadgesFromReportsFailurePreserveOutputTest() {
        sandboxServer.when(HttpRequest.request("/v1.0/task/" + TASK_ID)
                .withMethod(HttpMethod.GET.name()))
                .respond(SandboxTestData.responseFromResource("tasklet_with_reports_failure.json"));

        var badges = List.of(
                SandboxTaskBadgesConfig.of("build_report", "SAMOGON"),
                SandboxTaskBadgesConfig.of("collided_id")
        );

        var taskOutput = sandboxClient.getTask(TASK_ID).getData();
        var url = "http://some.task.ru/task_id";
        var result = sandboxTaskService.toTaskletResult(taskOutput, taskBadgeService, badges, url, true);
        assertThat(result.isSuccess()).isFalse();

        var expectOutput = JsonParser.parseString("""
                {
                   "sumValue": 3,
                   "@type": "type.googleapis.com/SimpleExample.Output"
                }
                """);
        assertThat(result.getOutput()).isEqualTo(expectOutput);

        var expected = List.of(
                TaskBadge.of(
                        "build_report", "SAMOGON", "http://some.task.ru/task_id/build_report",
                        TaskBadge.TaskStatus.SUCCESSFUL, null, null, false
                ),
                TaskBadge.of(
                        "collided_id", "SAMOGON", "http://some.url.ru/",
                        TaskBadge.TaskStatus.SUCCESSFUL, 1.0f, null, false
                )
        );
        assertThat(result.getTaskBadges()).isEqualTo(expected);
    }

    static List<Arguments> getResources() {
        return List.of(
                Arguments.of("ya_package_resources.json", "sandbox/expected_resources.json",
                        List.of()),
                Arguments.of("ya_package_resources_mixed_state.json", "sandbox/expected_resources_mixed_state.json",
                        List.of(ResourceState.READY, ResourceState.NOT_READY)));
    }
}
