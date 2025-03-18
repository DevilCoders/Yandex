package ru.yandex.ci.tools.client;

import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcServiceImpl;
import ru.yandex.ci.core.spring.clients.ArcClientConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Import(ArcClientConfig.class)
@Configuration
public class ArcClientTest extends AbstractSpringBasedApp {

    @Autowired
    ArcServiceImpl arcClient;

    @Override
    protected void run() throws Exception {
        testCommit();
    }

    void testCommit() {
        arcClient.getCommit(ArcRevision.of("e848d28ffcfeacd0a07974dde8f4cb98e3f83c75"));
    }

    void testBranches() {
        var branches = arcClient.getBranches("");
        log.info("getBranches: {}", branches.size());
    }

    public static void main(String[] args) {
        startAndStopThisClass(args, Environment.STABLE);
    }

}
