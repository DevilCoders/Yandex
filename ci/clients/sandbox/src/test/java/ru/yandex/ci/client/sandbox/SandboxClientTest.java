package ru.yandex.ci.client.sandbox;

import java.time.Duration;
import java.time.Instant;
import java.time.OffsetDateTime;
import java.util.ArrayList;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import io.netty.handler.codec.http.HttpMethod;
import org.assertj.core.util.Sets;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Nested;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.CsvSource;
import org.mockserver.client.MockServerClient;
import org.mockserver.junit.jupiter.MockServerExtension;
import org.mockserver.matchers.MatchType;
import org.mockserver.model.Header;
import org.mockserver.model.HttpResponse;
import org.mockserver.model.HttpStatusCode;
import org.mockserver.model.MediaType;

import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.OAuthProvider;
import ru.yandex.ci.client.sandbox.api.BatchResult;
import ru.yandex.ci.client.sandbox.api.BatchStatus;
import ru.yandex.ci.client.sandbox.api.DelegationResultList;
import ru.yandex.ci.client.sandbox.api.ResourceInfo;
import ru.yandex.ci.client.sandbox.api.ResourceState;
import ru.yandex.ci.client.sandbox.api.Resources;
import ru.yandex.ci.client.sandbox.api.SandboxBatchAction;
import ru.yandex.ci.client.sandbox.api.SandboxCustomField;
import ru.yandex.ci.client.sandbox.api.SandboxTask;
import ru.yandex.ci.client.sandbox.api.SandboxTaskOutput;
import ru.yandex.ci.client.sandbox.api.SandboxTaskPriority;
import ru.yandex.ci.client.sandbox.api.SandboxTaskPriority.PriorityClass;
import ru.yandex.ci.client.sandbox.api.SandboxTaskPriority.PrioritySubclass;
import ru.yandex.ci.client.sandbox.api.SandboxTaskRequirements;
import ru.yandex.ci.client.sandbox.api.SandboxTaskStatus;
import ru.yandex.ci.client.sandbox.api.SecretList;
import ru.yandex.ci.client.sandbox.api.TaskAuditRecord;
import ru.yandex.ci.client.sandbox.api.TaskExecution;
import ru.yandex.ci.client.sandbox.api.TaskRequirementsDns;
import ru.yandex.ci.client.sandbox.api.TaskRequirementsRamdisk;
import ru.yandex.ci.client.sandbox.api.notification.NotificationSetting;
import ru.yandex.ci.client.sandbox.api.notification.NotificationStatus;
import ru.yandex.ci.client.sandbox.api.notification.NotificationTransport;
import ru.yandex.ci.client.sandbox.model.Semaphore;
import ru.yandex.ci.util.ResourceUtils;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockserver.model.HttpRequest.request;
import static org.mockserver.model.HttpResponse.response;
import static org.mockserver.model.JsonBody.json;

@ExtendWith(MockServerExtension.class)
class SandboxClientTest {

    private final MockServerClient server;
    private SandboxClient sandboxClient;
    private static final String SEMAPHORE_ID = "724";
    private static final int RESOURCE_ID = 742;
    private static final int TASK_ID = 256;

    private static final int API_QUOTA_MILLIS = 4200;
    private static final int API_QUOTA_CONSUMPTION_MILLIS = 1000;
    private static final Header API_QUOTA_HEADER = Header.header(SandboxClientImpl.API_QUOTA_HEADER, API_QUOTA_MILLIS);
    private static final Header API_QUOTA_CONSUMPTION_HEADER = Header.header(
            SandboxClientImpl.API_QUOTA_CONSUMPTION_HEADER, API_QUOTA_CONSUMPTION_MILLIS
    );
    private static final List<Header> API_QUOTA_HEADERS = List.of(API_QUOTA_HEADER, API_QUOTA_CONSUMPTION_HEADER);

    SandboxClientTest(MockServerClient server) {
        this.server = server;
    }

    @BeforeEach
    void setUp() {
        server.reset();
        var properties = HttpClientProperties.builder()
                .endpoint("")
                .authProvider(new OAuthProvider("no-token"))
                .build();
        sandboxClient = SandboxClientImpl.create(
                SandboxClientProperties.builder()
                        .sandboxApiUrl("http:/" + server.remoteAddress() + "/api/v1.0/")
                        .sandboxApiV2Url("http:/" + server.remoteAddress() + "/api/v2/")
                        .httpClientProperties(properties)
                        .maxLimitForGetTasksRequest(1)
                        .build()
        );
    }

    @ParameterizedTest
    @CsvSource({
            "semaphore-audit-short-first.json,2019-08-14T21:00:04.243",
            "semaphore-audit-short-second.json,2019-08-14T19:00:07",
    })
    public void parseLongDate(String testFileName, String expected) {
        server.when(
                request("/api/v1.0/semaphore/" + SEMAPHORE_ID)
                        .withMethod(HttpMethod.GET.name()))
                .respond(
                        response(resource("semaphore.json"))
                                .withStatusCode(HttpStatusCode.OK_200.code())
                );

        server.when(
                request("/api/v1.0/semaphore/" + SEMAPHORE_ID + "/audit")
                        .withMethod(HttpMethod.GET.name()))
                .respond(
                        response(resource(testFileName))
                                .withStatusCode(HttpStatusCode.OK_200.code()));


        Semaphore semaphore = sandboxClient.getSemaphore(SEMAPHORE_ID);
        assertThat(semaphore.getUpdateTime()).isEqualTo(expected);
    }

    @Test
    public void resourceInfo() {
        server.when(request("/api/v1.0/resource/" + RESOURCE_ID).withMethod(HttpMethod.GET.name()))
                .respond(response(resource("resource-info.json")));

        ResourceInfo resourceInfo = sandboxClient.getResourceInfo(RESOURCE_ID);

        assertThat(resourceInfo).isNotNull();
        assertThat(resourceInfo.getId()).isEqualTo(1332730979);
        assertThat(resourceInfo.getType()).isEqualTo("SANDBOX_TASKS_BINARY");
        assertThat(resourceInfo.getState()).isEqualTo(ResourceState.READY);
        assertThat(resourceInfo.getAttributes())
                .containsOnlyKeys(
                        "binary_age",
                        "commit_revision",
                        "taskbox_enabled",
                        "backup_task",
                        "binary_hash",
                        "schema_Sawmill",
                        "schema_RemoteExec"
                ).containsEntry("binary_hash", "a88b8e30bd95cc4283f3ef3409fdfeca")
                .doesNotContainValue(null);
    }

    @Test
    void startTask() {
        server.when(request("/api/v1.0/batch/tasks/start")
                .withBody(json(resource("batch-tasks-request.json")))
                .withMethod(HttpMethod.PUT.name()))
                .respond(response(resource("batch-tasks-start-response.json")));

        var taskIds = new LinkedHashSet<>(List.of(973810836L, 973810997L, 973811034L));
        var results = sandboxClient.executeBatchTaskAction(SandboxBatchAction.START, taskIds, "Do action from test");

        assertThat(results).isEqualTo(List.of(
                new BatchResult(973810836, "Task started successfully.", BatchStatus.SUCCESS),
                new BatchResult(973810997, "Error starting task.", BatchStatus.ERROR),
                new BatchResult(973811034, "Task started successfully.", BatchStatus.SUCCESS)
        ));
    }

    @Test
    void stopTask() {
        server.when(request("/api/v1.0/batch/tasks/stop")
                .withBody(json(resource("batch-tasks-request.json")))
                .withMethod(HttpMethod.PUT.name()))
                .respond(response(resource("batch-tasks-stop-response.json")));

        var taskIds = Sets.newLinkedHashSet(973810836L, 973810997L, 973811034L);
        var results = sandboxClient.executeBatchTaskAction(SandboxBatchAction.STOP, taskIds, "Do action from test");

        assertThat(results).isEqualTo(List.of(
                new BatchResult(973810836, "Task #973810836 in status SUCCESS cannot be stopped.", BatchStatus.WARNING),
                new BatchResult(973810997, "Task scheduled for stop.", BatchStatus.SUCCESS),
                new BatchResult(973811034, "Task not found.", BatchStatus.ERROR)
        ));
    }

    @Test
    void getTaskList() {
        server.when(request("/api/v1.0/task")
                .withMethod(HttpMethod.GET.name())
                .withQueryStringParameter("id", "973810997,973811034")
                .withQueryStringParameter("limit", "2")
                .withQueryStringParameter("offset", "0")
                .withQueryStringParameter("fields", "execution,id,status")
        ).respond(responseFromResource("task-response.json"));

        assertThat(sandboxClient.getTasks(Sets.newLinkedHashSet(973810997L, 973811034L),
                Set.of("id", "status", "execution")))
                .isEqualTo(Map.of(
                        973810997L, SandboxTaskOutput.builder()
                                .id(973810997L)
                                .status(SandboxTaskStatus.STOPPED)
                                .execution(TaskExecution.builder()
                                        .started(Instant.parse("2021-05-19T12:52:21Z"))
                                        .finished(Instant.parse("2021-05-19T12:53:09Z"))
                                        .build()
                                )
                                .build(),
                        973811034L, SandboxTaskOutput.builder()
                                .id(973811034L)
                                .status(SandboxTaskStatus.FAILURE)
                                .execution(TaskExecution.builder()
                                        .started(Instant.parse("2021-05-19T12:52:29Z"))
                                        .finished(Instant.parse("2021-05-19T12:53:02Z"))
                                        .build()
                                )
                                .build()
                ));
    }

    @Test
    void getTasksWithConsumer() {
        server.when(request("/api/v1.0/task")
                .withMethod(HttpMethod.GET.name())
                .withQueryStringParameter("id", "973810997")
                .withQueryStringParameter("limit", "1")
                .withQueryStringParameter("offset", "0")
                .withQueryStringParameter("fields", "execution,id,status")
        )
                .respond(response(resource("task-response-973810997.json")).withContentType(MediaType.JSON_UTF_8));

        server.when(request("/api/v1.0/task")
                .withMethod(HttpMethod.GET.name())
                .withQueryStringParameter("id", "973811034")
                .withQueryStringParameter("limit", "1")
                .withQueryStringParameter("offset", "0")
                .withQueryStringParameter("fields", "execution,id,status")
        )
                .respond(response(resource("task-response-973811034.json")).withContentType(MediaType.JSON_UTF_8));

        List<Map<Long, SandboxTaskOutput>> calls = new ArrayList<>();
        sandboxClient.getTasks(Sets.newLinkedHashSet(973810997L, 973811034L),
                Sets.newLinkedHashSet("id", "status", "execution"), calls::add);

        assertThat(calls).containsExactly(
                Map.of(973810997L, SandboxTaskOutput.builder()
                        .id(973810997L)
                        .status(SandboxTaskStatus.STOPPED)
                        .execution(TaskExecution.builder()
                                .started(Instant.parse("2021-05-19T12:52:21Z"))
                                .finished(Instant.parse("2021-05-19T12:53:09Z"))
                                .build()
                        )
                        .build()
                ),
                Map.of(973811034L, SandboxTaskOutput.builder()
                        .id(973811034L)
                        .status(SandboxTaskStatus.FAILURE)
                        .execution(TaskExecution.builder()
                                .started(Instant.parse("2021-05-19T12:52:29Z"))
                                .finished(Instant.parse("2021-05-19T12:53:02Z"))
                                .build()
                        )
                        .build()
                )
        );
    }

    @Test
    void getTask() {
        server.when(request("/api/v1.0/task/973810836")
                .withMethod(HttpMethod.GET.name()))
                .respond(responseFromResource("get-task-response.json"));

        var task = sandboxClient.getTask(973810836);
        assertThat(task.getData()).isEqualTo(
                SandboxTaskOutput.builder()
                        .type("TASKLET_WOODCUTTER")
                        .id(973810836L)
                        .status(SandboxTaskStatus.SUCCESS)
                        .author("pochemuto")
                        .description("woodcutter [1], flow: ci::release-sawmill, " +
                                "flow-launch: 2a38fddeb17e986168bcfa21aa00f9b15e72afaf43751be77a158404ca6dcb06")
                        .inputParameters(Map.of(
                                "__tasklet_name__", "WoodcutterPy",
                                "__tasklet_input__", Map.of(
                                        "input_data", List.of("one", 13.5)
                                )
                        ))
                        .outputParameters(Map.of(
                                "__tasklet_output__", Map.of(
                                        "timbers", List.of(
                                                Map.of("name", "бревно из дерева Бамбук обыкновенный"),
                                                Map.of("name", "бревно из дерева Ясень высокий")
                                        )
                                ),
                                "tasklet_result",
                                """
                                        {
                                            "output": {
                                                "@type": "type.googleapis.com/WoodflowCi.woodcutter.Output",
                                                "timbers": [
                                                    {
                                                        "name": "бревно из дерева Бамбук обыкновенный"
                                                    },
                                                    {
                                                        "name": "бревно из дерева Ясень высокий"
                                                    }
                                                ]
                                            },
                                            "success": true
                                        }"""
                        ))
                        .reports(List.of(new SandboxTaskOutput.Report(
                                "https://sandbox.yandex-team.ru/api/v1.0/task/973810836/reports/footer",
                                "footer", "Footer"
                        )))
                        .execution(TaskExecution.builder()
                                .started(Instant.parse("2021-05-19T12:52:14Z"))
                                .finished(Instant.parse("2021-05-19T12:52:43Z"))
                                .build())
                        .tags(List.of("CI", "CI::RELEASE-SAWMILL", "LAUNCH:2A38FDDEB17E"))
                        .build()
        );
    }

    @Test
    void getTotalFilteredTasks() {
        server.when(request("/api/v1.0/task")
                .withMethod(HttpMethod.GET.name())
                .withQueryStringParameter("id", "974835295,974835344,974835391")
                .withQueryStringParameter("limit", "0")
                .withQueryStringParameter("offset", "0")
                .withQueryStringParameter("status", "STOPPING,STOPPED")
                .withQueryStringParameter("children", "true")
                .withQueryStringParameter("hidden", "true")
                .withQueryStringParameter("created", "1970-01-01T00:00:00Z..1970-01-15T00:00:00Z")
        )
                .respond(response(resource("get-filtered-tasks.json")).withContentType(MediaType.JSON_UTF_8));

        var filter = TasksFilter.builder()
                .created(TimeRange.of(
                        OffsetDateTime.parse("1970-01-01T00:00:00Z"),
                        OffsetDateTime.parse("1970-01-15T00:00:00Z"))
                )
                .ids(List.of(974835391L, 974835344L, 974835295L))
                .status(SandboxTaskStatus.STOPPING)
                .status(SandboxTaskStatus.STOPPED)
                .children(true)
                .hidden(true)
                .build();

        assertThat(sandboxClient.getTotalFilteredTasks(filter)).isEqualTo(110005L);
    }

    @Test
    void getTasksIdsFiltered() {
        server.when(request("/api/v1.0/task")
                .withMethod(HttpMethod.GET.name())
                .withQueryStringParameter("fields", "id")
                .withQueryStringParameter("limit", "98")
                .withQueryStringParameter("offset", "1")
                .withQueryStringParameter("type", "AUTOCHECK_BUILD_PARENT_2,AUTOCHECK_BUILD_YA_2")
                .withQueryStringParameter("all_tags", "true")
                .withQueryStringParameter("created", "1970-01-01T00:00:00Z..1970-01-15T00:00:00Z")
        )
                .respond(response(resource("get-filtered-tasks.json")).withContentType(MediaType.JSON_UTF_8));

        var filter = TasksFilter.builder()
                .created(TimeRange.of(
                        OffsetDateTime.parse("1970-01-01T00:00:00Z"),
                        OffsetDateTime.parse("1970-01-15T00:00:00Z")
                ))
                .type("AUTOCHECK_BUILD_YA_2")
                .type("AUTOCHECK_BUILD_PARENT_2")
                .allTags(true)
                .limit(98)
                .offset(1)
                .build();

        assertThat(sandboxClient.getTasksIds(filter))
                .containsExactly(974835391L, 974835344L, 974835295L);
    }

    @Test
    void getTasksFiltered() {
        server.when(request("/api/v1.0/task")
                .withMethod(HttpMethod.GET.name())
                .withQueryStringParameter("fields", "id")
                .withQueryStringParameter("limit", "98")
                .withQueryStringParameter("offset", "1")
                .withQueryStringParameter("type", "AUTOCHECK_BUILD_PARENT_2,AUTOCHECK_BUILD_YA_2")
                .withQueryStringParameter("all_tags", "true")
                .withQueryStringParameter("created", "1970-01-01T00:00:00Z..1970-01-15T00:00:00Z")
        )
                .respond(response(resource("get-filtered-tasks.json")).withContentType(MediaType.JSON_UTF_8));

        var filter = TasksFilter.builder()
                .field("id")
                .created(TimeRange.of(
                        OffsetDateTime.parse("1970-01-01T00:00:00Z"),
                        OffsetDateTime.parse("1970-01-15T00:00:00Z")
                ))
                .type("AUTOCHECK_BUILD_YA_2")
                .type("AUTOCHECK_BUILD_PARENT_2")
                .allTags(true)
                .limit(98)
                .offset(1)
                .build();

        assertThat(sandboxClient.getTasks(filter))
                .map(SandboxTaskOutput::getId)
                .containsExactly(974835391L, 974835344L, 974835295L);
    }

    @Test
    public void taskResources() {
        server.when(request("/api/v2/resource")
                .withMethod(HttpMethod.GET.name())
                .withQueryStringParameter("limit", "1000")
                .withQueryStringParameter("task_id", String.valueOf(TASK_ID))
                .withQueryStringParameter("type", "MY_RESOURCE")
        )
                .respond(response(resource("task-resources.json")));

        Resources resourceInfo = sandboxClient.getTaskResources(TASK_ID, "MY_RESOURCE");

        assertThat(resourceInfo).isNotNull();
        assertThat(resourceInfo).hasNoNullFieldsOrProperties();
        assertThat(resourceInfo.getItems()).hasSize(4);
        assertThat(resourceInfo.getItems())
                .extracting(ResourceInfo::getAttributes)
                .doesNotContainNull();
        assertThat(resourceInfo.getItems().get(3).getAttributes())
                .containsEntry("platform", "Linux-4.9.151-35-x86_64-with-Ubuntu-12.04-precise");
    }

    @Test
    void taskStatus() {
        server.when(request("/api/v1.0/task")
                .withQueryStringParameter("limit", String.valueOf(1))
                .withQueryStringParameter("fields", "status")
                .withQueryStringParameter("id", String.valueOf(TASK_ID))
                .withMethod(HttpMethod.GET.name()))
                .respond(responseFromResource("batch-task-status.json"));


        SandboxResponse<SandboxTaskStatus> taskStatusResponse = sandboxClient.getTaskStatus(TASK_ID);

        assertThat(taskStatusResponse).isEqualTo(sandboxResponse(SandboxTaskStatus.SUCCESS));
        assertThat(taskStatusResponse.getApiQuotaConsumptionPercent()).isEqualTo(23);
    }

    private static <T> SandboxResponse<T> sandboxResponse(T data) {
        return SandboxResponse.of(data, API_QUOTA_CONSUMPTION_MILLIS, API_QUOTA_MILLIS);
    }

    @Test
    void taskStatusUnknown() {
        server.when(request("/api/v1.0/task")
                .withQueryStringParameter("limit", String.valueOf(1))
                .withQueryStringParameter("fields", "status")
                .withQueryStringParameter("id", String.valueOf(TASK_ID))
                .withMethod(HttpMethod.GET.name()))
                .respond(responseFromResource("batch-task-status-unknown.json"));

        assertThat(sandboxClient.getTaskStatus(TASK_ID)).isEqualTo(sandboxResponse(SandboxTaskStatus.UNKNOWN));
    }

    @Test
    void taskAudit() {
        server.when(request("/api/v1.0/task/audit")
                .withQueryStringParameter("id", String.valueOf(TASK_ID))
                .withMethod(HttpMethod.GET.name()))
                .respond(responseFromResource("task-audit.json"));

        assertThat(sandboxClient.getTaskAudit(TASK_ID))
                .isEqualTo(sandboxResponse(List.of(
                        new TaskAuditRecord(
                                SandboxTaskStatus.DRAFT,
                                Instant.parse("2021-03-23T18:57:38.641000Z"),
                                "Created",
                                "robot-ci"
                        ),
                        new TaskAuditRecord(
                                SandboxTaskStatus.PREPARING,
                                Instant.parse("2021-03-23T18:57:57.001000Z"),
                                "Executing on sandbox588 (linux_ubuntu_12.04_precise)",
                                "robot-ci"
                        ),
                        new TaskAuditRecord(
                                SandboxTaskStatus.SUCCESS,
                                Instant.parse("2021-03-23T19:05:05.004000Z"),
                                null,
                                null
                        )
                )));
    }

    @Test
    void getTasks_withResponseConsumer() {
        Set.of(900221842L, 909718867L, 910688325L).forEach(sbTaskId ->
                server.when(request("/api/v1.0/task")
                        .withQueryStringParameter("limit", String.valueOf(1))
                        .withQueryStringParameter("offset", String.valueOf(0))
                        .withQueryStringParameter("fields", "id,status")
                        .withQueryStringParameter("id", String.valueOf(sbTaskId))
                        .withMethod(HttpMethod.GET.name()))
                        .respond(response(resource("get-tasks-" + sbTaskId + ".json")))
        );

        List<Long> ids = new ArrayList<>();
        int[] callCount = new int[]{0};
        sandboxClient.getTasks(
                Set.of(900221842L, 909718867L, 910688325L),
                Set.of("id", "status"),
                response -> {
                    response.values()
                            .stream()
                            .map(SandboxTaskOutput::getId)
                            .forEach(ids::add);
                    callCount[0]++;
                }
        );
        assertThat(callCount[0]).isEqualTo(3);
        assertThat(ids).contains(900221842L, 909718867L, 910688325L);
    }

    @Nested
    class CreateTask {

        @Test
        void mappedResults() {
            server.when(request("/api/v1.0/task").withMethod(HttpMethod.POST.name()))
                    .respond(response(resource("create-task.json")));

            var sandboxTask = SandboxTask.builder().build();
            SandboxTaskOutput output = sandboxClient.createTask(sandboxTask);
            assertThat(output.getId()).isEqualTo(771708697);
            assertThat(output.getType()).isEqualTo("TASKLET_WOODCUTTER");
            assertThat(output.getAuthor()).isEqualTo("pochemuto");
            assertThat(output.getDescription()).isEqualTo("Test manual creation");
            assertThat(output.getStatusEnum()).isEqualTo(SandboxTaskStatus.DRAFT);
            assertThat(output.getInputParameters().get("ttl")).isEqualTo(Double.POSITIVE_INFINITY);
        }

        @Test
        void defaultRequirementsPassed() {
            server.when(request("/api/v1.0/task").withMethod(HttpMethod.POST.name()))
                    .respond(response(resource("create-task.json")));

            SandboxTask sandboxTask = SandboxTask.builder()
                    .requirements(requirements())
                    .build();
            sandboxClient.createTask(sandboxTask);

            server.verify(request("/api/v1.0/task")
                    .withBody(json("""
                                    {
                                        "custom_fields":[],
                                        "context":{},
                                        "requirements":{}
                                    }
                                    """,
                            MatchType.STRICT)));
        }

        @Test
        void killTimeoutPassed() {
            server.when(request("/api/v1.0/task").withMethod(HttpMethod.POST.name()))
                    .respond(response(resource("create-task.json")));

            var sandboxTask = SandboxTask.builder()
                    .killTimeoutDuration(Duration.ofHours(2).plusSeconds(17))
                    .build();

            sandboxClient.createTask(sandboxTask);

            server.verify(request("/api/v1.0/task").withMethod(HttpMethod.POST.name())
                    .withBody(json("""
                            {
                                "custom_fields" : [ ],
                                "context" : { },
                                "kill_timeout" : 7217
                            }
                            """)));
        }

        @Test
        void customFields() {
            server.when(request("/api/v1.0/task").withMethod(HttpMethod.POST.name()))
                    .respond(response(resource("create-task.json")));

            SandboxTask sandboxTask = SandboxTask.builder().build();

            Map<String, Object> input = Map.of(
                    "boards", List.of(
                            Map.of("seq", 4, "source", Map.of("name", "береза"), "producer", "лесопилка"),
                            Map.of("seq", 8, "source", Map.of("name", "липа"), "producer", "лесопилка 17")
                    )
            );
            sandboxTask.getCustomFields().add(new SandboxCustomField("test", Map.of("tasklet_input", input)));

            sandboxClient.createTask(sandboxTask);

            server.verify(request("/api/v1.0/task").withMethod(HttpMethod.POST.name())
                    .withBody(json("""
                            {
                                "context": {},
                                "custom_fields": [
                                    {
                                        "name": "test",
                                        "value": {
                                            "tasklet_input": {
                                                "boards": [
                                                    {
                                                        "seq": 4,
                                                        "source": {
                                                            "name": "береза"
                                                        },
                                                        "producer": "лесопилка"
                                                    },
                                                    {
                                                        "seq": 8,
                                                        "source": {
                                                            "name": "липа"
                                                        },
                                                        "producer": "лесопилка 17"
                                                    }
                                                ]
                                            }
                                        }
                                    }
                                ]
                            }
                            """)));
        }

        @Test
        void requirementsMapping() {
            server.when(request("/api/v1.0/task").withMethod(HttpMethod.POST.name()))
                    .respond(response(resource("create-task.json")));

            SandboxTask sandboxTask = SandboxTask.builder().requirements(
                    requirements()
                            .setCores(3L)
                            .setTasksResource(9L)
                            .setDiskSpace(456L)
                            .setRam(678L)
                            .setDns(TaskRequirementsDns.DNS64)
                            .setClientTags("LINUX | WINDOWS")
                            .setContainerResource(45L)
                            .setCpuModel("x86")
                            .setHost("my-host")
                            .setPrivileged(false)
                            .setPlatform("linux")
            ).build();
            sandboxClient.createTask(sandboxTask);

            server.verify(request("/api/v1.0/task")
                    .withBody(json("""
                                    {
                                        "custom_fields":[],
                                        "context":{},
                                        "requirements": {
                                            "client_tags":"LINUX | WINDOWS",
                                            "container_resource":45,
                                            "cores":3,
                                            "cpu_model":"x86",
                                            "disk_space":456,
                                            "dns":"dns64",
                                            "host":"my-host",
                                            "platform":"linux",
                                            "privileged":false,
                                            "ram":678,
                                            "tasks_resource":9
                                        }
                                    }
                                    """,
                            MatchType.STRICT)));
        }

        @Test
        void defaultPriorityPassed() {
            server.when(request("/api/v1.0/task").withMethod(HttpMethod.POST.name()))
                    .respond(response(resource("create-task.json")));

            SandboxTask sandboxTask = SandboxTask.builder()
                    .requirements(requirements())
                    .priority(new SandboxTaskPriority())
                    .build();
            sandboxClient.createTask(sandboxTask);

            server.verify(request("/api/v1.0/task")
                    .withBody(json("""
                                    {
                                        "custom_fields":[],
                                        "context":{},
                                        "priority":{},
                                        "requirements":{}
                                    }
                                    """,
                            MatchType.STRICT)));
        }

        @Test
        void priorityMapping() {
            server.when(request("/api/v1.0/task").withMethod(HttpMethod.POST.name()))
                    .respond(response(resource("create-task.json")));

            SandboxTask sandboxTask = SandboxTask.builder()
                    .requirements(requirements())
                    .priority(new SandboxTaskPriority(PriorityClass.SERVICE, PrioritySubclass.HIGH))
                    .build();
            sandboxClient.createTask(sandboxTask);

            server.verify(request("/api/v1.0/task")
                    .withBody(json("""
                                    {
                                        "custom_fields":[],
                                        "context":{},
                                        "priority":{
                                            "class":"SERVICE",
                                            "subclass":"HIGH"
                                        },
                                        "requirements":{}
                                    }
                                    """,
                            MatchType.STRICT)));
        }

        @Test
        void requireRamDriveMapping() {
            server.when(request("/api/v1.0/task").withMethod(HttpMethod.POST.name()))
                    .respond(response(resource("create-task.json")));

            SandboxTask sandboxTask = SandboxTask.builder()
                    .requirements(
                            requirements().setRamdrive(new TaskRequirementsRamdisk().setSize(78))
                    ).build();
            sandboxClient.createTask(sandboxTask);

            server.verify(request("/api/v1.0/task")
                    .withBody(json("""
                                    {
                                        "custom_fields":[],
                                        "context":{},
                                        "requirements":{
                                            "ramdrive":{
                                                "size":78,
                                                "type":"tmpfs"
                                            }
                                        }
                                    }
                                    """,
                            MatchType.STRICT)));
        }

        @Test
        void taskPriority() {
            server.when(request("/api/v1.0/task").withMethod(HttpMethod.POST.name()))
                    .respond(response(resource("create-task.json")));

            SandboxTask sandboxTask = SandboxTask.builder()
                    .priority(new SandboxTaskPriority(
                            PriorityClass.SERVICE,
                            PrioritySubclass.HIGH
                    ))
                    .build();
            sandboxClient.createTask(sandboxTask);

            server.verify(request("/api/v1.0/task")
                    .withBody(json("""
                                    {
                                        "custom_fields":[],
                                        "context":{},
                                        "priority":{
                                            "class":"SERVICE",
                                            "subclass":"HIGH"
                                        }
                                    }
                                    """,
                            MatchType.STRICT
                    )));
        }

        @Test
        void notifications() {
            server.when(request("/api/v1.0/task").withMethod(HttpMethod.POST.name()))
                    .respond(response(resource("create-task.json")));

            SandboxTask sandboxTask = SandboxTask.builder()
                    .priority(new SandboxTaskPriority(
                            PriorityClass.SERVICE,
                            PrioritySubclass.HIGH
                    ))
                    .notifications(List.of(
                            new NotificationSetting(NotificationTransport.TELEGRAM, List.of(NotificationStatus.DELETED),
                                    List.of("user1")
                            ),
                            new NotificationSetting(NotificationTransport.EMAIL, List.of(
                                    NotificationStatus.ASSIGNED, NotificationStatus.FAILURE),
                                    List.of("user2", "user3")
                            ),
                            new NotificationSetting(NotificationTransport.Q, List.of(
                                    NotificationStatus.ASSIGNED, NotificationStatus.FAILURE),
                                    List.of("user4")
                            )
                    )).build();
            sandboxClient.createTask(sandboxTask);

            server.verify(request("/api/v1.0/task")
                    .withBody(json("""
                                    {
                                      "context": {},
                                      "priority": {
                                        "class": "SERVICE",
                                        "subclass": "HIGH"
                                      },
                                      "notifications": [
                                        {
                                          "transport": "telegram",
                                          "statuses": [
                                            "DELETED"
                                          ],
                                          "recipients": [
                                            "user1"
                                          ]
                                        },
                                        {
                                          "transport": "email",
                                          "statuses": [
                                            "ASSIGNED",
                                            "FAILURE"
                                          ],
                                          "recipients": [
                                            "user2",
                                            "user3"
                                          ]
                                        },
                                        {
                                          "transport": "q",
                                          "statuses": [
                                            "ASSIGNED",
                                            "FAILURE"
                                          ],
                                          "recipients": [
                                            "user4"
                                          ]
                                        }
                                      ],
                                      "custom_fields": []
                                    }
                                    """,
                            MatchType.STRICT
                    )));
        }
    }

    @Test
    void delegateTokens() {
        server.when(request("/api/v1.0/yav/tokens")
                .withBody(json(resource("yav-tokens-put-request.json")))
                .withMethod(HttpMethod.POST.name())
                .withHeader("X-Ya-User-Ticket", "user-ticket-004")
        )
                .respond(response(resource("yav-tokens-put-response.json")));

        var result = sandboxClient.delegateYavSecrets(SecretList.withSecrets(
                "sec-01e8agdtdcs61v6emr05h5q1ek",
                "sec-01e8agdtdcs61v6emr05h5q1e0"
        ), "user-ticket-004");

        assertThat(result).isEqualTo(
                new DelegationResultList(List.of(
                        new DelegationResultList.DelegationResult(
                                true, "ok", "sec-01e8agdtdcs61v6emr05h5q1ek"
                        ),
                        new DelegationResultList.DelegationResult(
                                false, "HTTP 400 from Yav: Requested a non-existent entity " +
                                "(Secret, sec-01e8agdtdcs61v6emr05h5q1e0)", "sec-01e8agdtdcs61v6emr05h5q1e0"
                        )
                ))
        );
    }

    @Test
    void getCurrentUserGroups() {
        server.when(request("/api/v1.0/user/current/groups").withMethod(HttpMethod.GET.name()))
                .respond(response(resource("user-current-groups-response.json")));

        assertThat(sandboxClient.getCurrentUserGroups())
                .containsExactly("AUTOCHECK", "AUTOCHECK-FAT");
    }

    @Test
    void getCurrentLogin() {
        server.when(request("/api/v1.0/user/current").withMethod(HttpMethod.GET.name()))
                .respond(response(resource("user-current-response.json")));

        assertThat(sandboxClient.getCurrentLogin()).isEqualTo("pochemuto");
    }

    @Test
    void setSemaphoreCapacity() {
        server.when(request("/api/v1.0/semaphore/" + 35855001).withMethod(HttpMethod.PUT.name())
                .withBody(json(resource("semaphore-put-request.json")))
        )
                .respond(response(resource("semaphore-put-response.json")));

        sandboxClient.setSemaphoreCapacity("35855001", 7, "Test event message");
    }

    @Test
    void setSemaphore204HttpCode() {
        server.when(request("/api/v1.0/semaphore/" + 35855001).withMethod(HttpMethod.PUT.name())
                .withBody(json(resource("semaphore-put-request.json")))
                .withHeader("X-Request-Updated-Data")
        )
                .respond(response(resource("semaphore-put-response.json")));

        sandboxClient.setSemaphoreCapacity("35855001", 7, "Test event message");
    }

    private SandboxTaskRequirements requirements() {
        return new SandboxTaskRequirements();
    }

    private static HttpResponse responseFromResource(String name) {
        return response(resource(name))
                .withHeaders(API_QUOTA_HEADERS)
                .withContentType(MediaType.JSON_UTF_8);
    }

    private static String resource(String name) {
        return ResourceUtils.textResource(name);
    }
}
