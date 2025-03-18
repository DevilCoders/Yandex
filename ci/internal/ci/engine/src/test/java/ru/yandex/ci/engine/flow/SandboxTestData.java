package ru.yandex.ci.engine.flow;

import java.time.Instant;
import java.util.List;
import java.util.Map;
import java.util.Random;

import javax.annotation.Nullable;

import one.util.streamex.StreamEx;
import org.mockserver.model.Headers;
import org.mockserver.model.HttpResponse;
import org.mockserver.model.MediaType;

import ru.yandex.ci.client.sandbox.SandboxResponse;
import ru.yandex.ci.client.sandbox.api.SandboxTaskOutput;
import ru.yandex.ci.client.sandbox.api.SandboxTaskStatus;
import ru.yandex.ci.client.sandbox.api.TaskAuditRecord;
import ru.yandex.ci.test.TestUtils;

import static org.mockserver.model.Header.header;
import static org.mockserver.model.HttpResponse.response;

public class SandboxTestData {

    public static final Headers QUOTA_HEADERS = new Headers(
            header("X-Api-Quota-Milliseconds", 1007),
            header("X-Api-Consumption-Milliseconds", 103)
    );

    private SandboxTestData() {
    }

    public static HttpResponse responseFromResource(String name) {
        return response(TestUtils.textResource("server_responses/sandbox/" + name))
                .withHeaders(QUOTA_HEADERS)
                .withContentType(MediaType.JSON_UTF_8);
    }

    public static SandboxResponse<SandboxTaskOutput> taskOutput(SandboxTaskStatus status) {
        return taskOutput(0, 100, status, null, null, null);
    }

    public static SandboxResponse<SandboxTaskOutput> uniqTaskOutput(SandboxTaskStatus status) {
        Random random = new Random();
        Map<String, Object> outputParameters = Map.of("param", random.nextInt());
        return taskOutput(0, 100, status, null, outputParameters, null);
    }

    public static SandboxResponse<SandboxTaskOutput> taskOutput(int apiQuotaConsumptionMillis,
                                                                int apiQuotaMillis,
                                                                SandboxTaskStatus status,
                                                                @Nullable Map<String, Object> inputParameters,
                                                                @Nullable Map<String, Object> outputParameters,
                                                                @Nullable List<SandboxTaskOutput.Report> reports) {
        SandboxTaskOutput taskOutput = new SandboxTaskOutput(
                "MY_TASK",
                -1,
                status,
                inputParameters,
                outputParameters,
                reports
        );

        return SandboxResponse.of(
                taskOutput,
                apiQuotaConsumptionMillis,
                apiQuotaMillis
        );
    }


    public static SandboxResponse<SandboxTaskStatus> taskStatus(SandboxTaskStatus status) {
        return taskStatus(0, 100, status);
    }

    public static SandboxResponse<SandboxTaskStatus> taskStatus(int apiQuotaConsumptionMillis,
                                                                int apiQuotaMillis,
                                                                SandboxTaskStatus status) {
        return SandboxResponse.of(
                status,
                apiQuotaConsumptionMillis,
                apiQuotaMillis
        );

    }

    public static SandboxResponse<List<TaskAuditRecord>> taskAudit(SandboxTaskStatus... statuses) {
        return taskAudit(0, 100, statuses);
    }

    public static SandboxResponse<List<TaskAuditRecord>> taskAudit(int apiQuotaConsumptionMillis,
                                                                   int apiQuotaMillis,
                                                                   SandboxTaskStatus... statuses) {
        return SandboxResponse.of(
                StreamEx.of(statuses)
                        .map(status -> new TaskAuditRecord(status, Instant.MAX, null, null))
                        .toList(),
                apiQuotaConsumptionMillis,
                apiQuotaMillis
        );

    }


}
