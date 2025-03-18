package ru.yandex.ci.tools.client;

import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.testenv.TestenvClient;
import ru.yandex.ci.engine.spring.clients.TestenvClientConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Import(TestenvClientConfig.class)
@Configuration
public class TestenvClientTest extends AbstractSpringBasedApp {

    @Autowired
    TestenvClient testenvClient;

    @Override
    protected void run() {
        log.info("Projects: {}", testenvClient.getProjects());

        log.info("Project: {}", testenvClient.getProject("zora-trunk"));
        log.info("Jobs: {}", testenvClient.getJobs("zora-trunk"));
    }

    public static void main(String[] args) {
        startAndStopThisClass(args);
    }

}
