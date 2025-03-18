package ru.yandex.ci.tools.client;

import java.time.Instant;
import java.time.temporal.ChronoUnit;
import java.util.Map;

import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.logs.LogsClient;
import ru.yandex.ci.core.spring.clients.LogsClientConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Configuration
@Import(LogsClientConfig.class)
public class LogsClientTest extends AbstractSpringBasedApp {

    @Autowired
    LogsClient logsClient;

    @Override
    protected void run() {
        var to = Instant.now();
        var from = to.minus(1, ChronoUnit.DAYS);

        var jobId = "full-curcuit-left-jobs-1-1";
        var taskId = "FlowJobWorkflow-fc3c1151f8c3286b060a08bc187aac00341e13cc1c0543be18277f60d823e481-" + jobId;
        var query = Map.of(
                "project", "ci",
                "service", "ci-tms",
                "task_id", taskId
        );

        var logs = logsClient.getLog(from, to, query, 10000);
        log.info("Found {} log records", logs.size());
    }

    public static void main(String[] args) {
        startAndStopThisClass(args);
    }
}
