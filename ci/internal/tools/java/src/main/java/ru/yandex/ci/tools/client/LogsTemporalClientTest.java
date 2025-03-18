package ru.yandex.ci.tools.client;

import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.common.temporal.logging.TemporalLogService;
import ru.yandex.ci.flow.spring.temporal.CiTemporalServiceConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Configuration
@Import(CiTemporalServiceConfig.class)
public class LogsTemporalClientTest extends AbstractSpringBasedApp {

    @Autowired
    TemporalLogService temporalLogService;

    @Override
    protected void run() {
        var id = "FlowJobWorkflow-8520d734c345d375dbf8bad030569a25b96962140980a56d8986c8b7795217b5-run-actions-1-1";
        var logs = temporalLogService.getLog(id);
        log.info(logs);
    }

    public static void main(String[] args) {
        startAndStopThisClass(args);
    }
}
