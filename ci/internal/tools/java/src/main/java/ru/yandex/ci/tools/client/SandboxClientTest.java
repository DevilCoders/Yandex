package ru.yandex.ci.tools.client;

import java.util.Set;

import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.core.job.JobResource;
import ru.yandex.ci.core.spring.clients.SandboxClientConfig;
import ru.yandex.ci.engine.SandboxTaskService;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Configuration
@Import(SandboxClientConfig.class)
public class SandboxClientTest extends AbstractSpringBasedApp {

    @Autowired
    private SandboxClient client;

    @Override
    protected void run() throws Exception {
        getResource();
    }

    void getResource() {
        log.info("Resource: {}", client.getResourceInfo(3243214296L));
        log.info("Resource: {}", client.getResourceInfo(3267689549L));
    }


    void getTaskResources() throws InterruptedException {
        var taskService = new SandboxTaskService(client, "CI", null);
        var taskId = 1193521014;

        var t = new Thread(() -> {
            var resources = taskService.getTaskResources(taskId, Set.of());
            log.info("Total {} resources", resources.size());
            for (JobResource resource : resources) {
                log.info("{}", resource);
            }
        });
        t.start();
        log.info("Interrupting....");
        t.interrupt();
        t.join();
    }

    void getTask() {
        var taskId = 1338412178;

        var output = client.getTask(taskId);
        log.info("Output: {}", output.getData());
    }

    public static void main(String[] args) {
        startAndStopThisClass(args);
    }


}
