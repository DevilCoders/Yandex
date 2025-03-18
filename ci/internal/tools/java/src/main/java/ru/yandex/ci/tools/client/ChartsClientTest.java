package ru.yandex.ci.tools.client;

import java.time.Instant;
import java.time.temporal.ChronoUnit;

import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.charts.ChartsClient;
import ru.yandex.ci.client.charts.model.ChartsGetCommentRequest;
import ru.yandex.ci.tms.spring.tasks.autocheck.degradation.ChartsClientConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Import(ChartsClientConfig.class)
@Configuration
public class ChartsClientTest extends AbstractSpringBasedApp {

    @Autowired
    ChartsClient chartsClient;

    @Override
    protected void run() throws Exception {
        testClient();
    }

    void testClient() {
        var comments = chartsClient.getComments(new ChartsGetCommentRequest(
                "ci/autocheck_stages_stat_window_new",
                Instant.now(),
                Instant.now().plus(2, ChronoUnit.DAYS)));

        log.info("Comments: {}", comments);
    }

    public static void main(String[] args) {
        startAndStopThisClass(args, Environment.STABLE);
    }

}
