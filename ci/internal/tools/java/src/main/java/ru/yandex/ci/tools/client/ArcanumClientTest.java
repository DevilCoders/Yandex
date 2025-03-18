package ru.yandex.ci.tools.client;

import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.arcanum.ArcanumClientImpl;
import ru.yandex.ci.engine.spring.clients.ArcanumClientConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Import(ArcanumClientConfig.class)
@Configuration
public class ArcanumClientTest extends AbstractSpringBasedApp {

    @Autowired
    ArcanumClientImpl arcanumClient;

    @Override
    protected void run() throws Exception {
        testClient();
    }

    void testClient() {
        log.info("getReviewRequest: {}", arcanumClient.getReviewRequest(1865427));
        log.info("getActiveDiffSet: {}", arcanumClient.getActiveDiffSet(1865427));
        log.info("getAllDiffSets: {}", arcanumClient.getAllDiffSets(1865427));
        log.info("getMergeRequirements: {}", arcanumClient.getMergeRequirements(1865427));
        log.info("getReviewRequestActivities: {}", arcanumClient.getReviewRequestActivities(1865427));
        log.info("getReviewRequestBySvnRevision: {}", arcanumClient.getReviewRequestBySvnRevision(8356573));
        log.info("getReviewRequestData: {}", arcanumClient.getReviewRequestData(1865427, "id"));
        log.info("getGroups: {}", arcanumClient.getGroups());
    }

    public static void main(String[] args) {
        startAndStopThisClass(args, Environment.STABLE);
    }

}
