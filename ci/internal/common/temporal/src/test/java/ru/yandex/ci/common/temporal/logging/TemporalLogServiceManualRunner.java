package ru.yandex.ci.common.temporal.logging;

import java.time.ZoneId;

import ru.yandex.ci.client.logs.LogsClientManualRunner;
import ru.yandex.ci.common.temporal.TemporalServiceManualRunner;

class TemporalLogServiceManualRunner {
    private TemporalLogServiceManualRunner() {
    }

    public static TemporalLogService createTemporalLogService() {
        var logsClient = LogsClientManualRunner.createLogsClient();
        var temporalService = TemporalServiceManualRunner.createTemporalService("ci");

        return new TemporalLogService(
                temporalService,
                logsClient,
                "ci",
                "ci-tms",
                "testing",
                ZoneId.systemDefault()
        );
    }

    public static void main(String[] args) {
        var logService = createTemporalLogService();

        var id = "FlowJobWorkflow-165657ca346fa28de152769a0ec33a69af58d8015115e52c5c71b63f9cc733c7-stable-tms-deploy-1";
        var logs = logService.getLog(id);
        System.out.println(logs);
    }

}
