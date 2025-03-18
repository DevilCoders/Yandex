package ru.yandex.ci.tms.task.autocheck.degradation.postcommit.task;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.Mockito;
import org.mockserver.integration.ClientAndServer;
import org.mockserver.model.HttpRequest;
import org.mockserver.model.Parameter;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.http.HttpMethod;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.CommonTestBase;
import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.client.sandbox.TasksFilter;
import ru.yandex.ci.client.sandbox.api.SandboxTaskStatus;
import ru.yandex.ci.tms.data.autocheck.SemaphoreId;
import ru.yandex.ci.tms.spring.clients.SandboxClientTestConfig;
import ru.yandex.ci.tms.task.autocheck.degradation.TestAutocheckDegradationUtils;
import ru.yandex.commune.bazinga.BazingaTaskManager;

@ContextConfiguration(classes = {
        SandboxClientTestConfig.class
})
public class AutocheckDegradationPostcommitTasksRestartSchedulerTaskTest extends CommonTestBase {
    private static final SemaphoreId TEST_SEMAPHORE = SemaphoreId.AUTOCHECK;
    private static final AutocheckDegradationPostcommitTasksRestartSchedulerTask.Parameters PARAMS =
            new AutocheckDegradationPostcommitTasksRestartSchedulerTask.Parameters(TEST_SEMAPHORE);

    private static final Map<String, String> TASKS_FILTER = Map.of(
            "type", "AUTOCHECK_BUILD_PARENT_2,AUTOCHECK_BUILD_YA_2",
            "status", "ASSIGNED,EXECUTING,PREPARING,WAIT_TASK",
            "children", "true",
            "hidden", "true",
            "tags", "TESTENV-AUTOCHECK-JOB",
            "all_tags", "true",
            "semaphore_acquirers", TEST_SEMAPHORE.getSandboxId());

    private static final TasksFilter TASKS_FILTERS = TasksFilter.builder()
            .type("AUTOCHECK_BUILD_YA_2")
            .type("AUTOCHECK_BUILD_PARENT_2")
            .status(SandboxTaskStatus.ASSIGNED)
            .status(SandboxTaskStatus.EXECUTING)
            .status(SandboxTaskStatus.WAIT_TASK)
            .status(SandboxTaskStatus.PREPARING)
            .children(true)
            .hidden(true)
            .tag("TESTENV-AUTOCHECK-JOB")
            .allTags(true)
            .semaphoreAcquirers(Integer.parseInt(TEST_SEMAPHORE.getSandboxId()))
            .build();
    private static final List<Parameter> FILTER_PARAMETERS = TASKS_FILTER.entrySet().stream()
            .map(e -> new Parameter(e.getKey(), e.getValue())).collect(Collectors.toList());

    @Autowired
    public ClientAndServer sandboxServer;

    @Autowired
    SandboxClient sandboxClient;

    @MockBean
    public BazingaTaskManager bazingaTaskManager;

    public AutocheckDegradationPostcommitTasksRestartSchedulerTask schedulerTask;

    @BeforeEach
    void setUp() {
        sandboxServer.clear(HttpRequest.request("/v1.0/task").withMethod(HttpMethod.GET.name()));
        sandboxServer.clear(HttpRequest.request("/v1.0/batch/tasks").withMethod(HttpMethod.PUT.name()));

        schedulerTask = new AutocheckDegradationPostcommitTasksRestartSchedulerTask(sandboxClient, bazingaTaskManager);
    }

    @Test
    void testGetRestartTasksIds() {
        sandboxServer.when(getTotalTasksRequest(FILTER_PARAMETERS))
                .respond(TestAutocheckDegradationUtils.getHttpResponseFromResource(
                        "server_responses/sandbox/get_task_total_250.json"));
        sandboxServer.when(getTasksIdsRequest(FILTER_PARAMETERS))
                .respond(TestAutocheckDegradationUtils.getHttpResponseFromResource(
                        "server_responses/sandbox/get_task_ids_total_250.json"));

        schedulerTask.execute(PARAMS, null);

        sandboxServer.verify(getTotalTasksRequest(FILTER_PARAMETERS), getTasksIdsRequest(FILTER_PARAMETERS));
    }

    @Test
    void testScheduleRestartTasks() {
        sandboxServer.when(getTasksIdsRequest(FILTER_PARAMETERS))
                .respond(TestAutocheckDegradationUtils.getHttpResponseFromResource(
                        "server_responses/sandbox/get_task_ids_total_250.json"));
        sandboxServer.when(getTotalTasksRequest(FILTER_PARAMETERS))
                .respond(TestAutocheckDegradationUtils.getHttpResponseFromResource(
                        "server_responses/sandbox/get_task_total_250.json"));

        var total = sandboxClient.getTotalFilteredTasks(TASKS_FILTERS);
        var filter = TASKS_FILTERS.toBuilder().limit(StrictMath.toIntExact(total)).build();
        List<Long> tasksIds = sandboxClient.getTasksIds(filter);

        schedulerTask.execute(PARAMS, null);

        ArgumentCaptor<AutocheckDegradationPostcommitTasksBaseTask> argument = ArgumentCaptor.forClass(
                AutocheckDegradationPostcommitTasksBaseTask.class);

        Mockito.verify(bazingaTaskManager, Mockito.times(3)).schedule(argument.capture());
        Assertions.assertEquals(Set.copyOf(tasksIds.subList(0, 100)),
                argument.getAllValues().get(0).getParametersTyped().getSandboxTasksIds());
        Assertions.assertEquals(Set.copyOf(tasksIds.subList(100, 200)),
                argument.getAllValues().get(1).getParametersTyped().getSandboxTasksIds());
        Assertions.assertEquals(Set.copyOf(tasksIds.subList(200, 250)),
                argument.getAllValues().get(2).getParametersTyped().getSandboxTasksIds());
    }

    private HttpRequest getTotalTasksRequest(List<Parameter> filterParameters) {
        List<Parameter> parameters = new ArrayList<>(filterParameters);
        parameters.add(Parameter.param("limit", "0"));

        return HttpRequest.request("/v1.0/task").withMethod(HttpMethod.GET.name())
                .withQueryStringParameters(parameters);
    }

    private HttpRequest getTasksIdsRequest(List<Parameter> filterParameters) {
        List<Parameter> parameters = new ArrayList<>(filterParameters);
        parameters.add(Parameter.param("limit", "250"));
        parameters.add(Parameter.param("fields", List.of("id")));

        return HttpRequest.request("/v1.0/task").withMethod(HttpMethod.GET.name())
                .withQueryStringParameters(parameters);
    }
}
