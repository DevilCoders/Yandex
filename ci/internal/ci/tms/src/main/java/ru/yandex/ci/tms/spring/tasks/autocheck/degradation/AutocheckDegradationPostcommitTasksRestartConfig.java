package ru.yandex.ci.tms.spring.tasks.autocheck.degradation;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.common.bazinga.spring.BazingaCoreConfig;
import ru.yandex.ci.core.spring.clients.SandboxClientConfig;
import ru.yandex.ci.tms.task.autocheck.degradation.postcommit.task.AutocheckDegradationPostcommitTasksRestartSchedulerTask;
import ru.yandex.ci.tms.task.autocheck.degradation.postcommit.task.AutocheckDegradationPostcommitTasksRestartTask;
import ru.yandex.ci.tms.task.autocheck.degradation.postcommit.task.AutocheckDegradationPostcommitTasksStartTask;
import ru.yandex.commune.bazinga.BazingaTaskManager;

@Configuration
@Import({
        BazingaCoreConfig.class,
        SandboxClientConfig.class
})

public class AutocheckDegradationPostcommitTasksRestartConfig {

    @Value("${ci.AutocheckDegradationPostcommitTasksRestartConfig.dryRun}")
    boolean dryRun;

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public AutocheckDegradationPostcommitTasksRestartSchedulerTask autocheckPostcommitTasksRestartSchedulerTask(
            SandboxClient sandboxClient,
            BazingaTaskManager bazingaTaskManager) {
        return new AutocheckDegradationPostcommitTasksRestartSchedulerTask(sandboxClient, bazingaTaskManager);
    }

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public AutocheckDegradationPostcommitTasksRestartTask autocheckPostcommitTasksRestartTask(
            SandboxClient sandboxClient,
            BazingaTaskManager bazingaTaskManager) {
        return new AutocheckDegradationPostcommitTasksRestartTask(dryRun, sandboxClient, bazingaTaskManager);
    }

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public AutocheckDegradationPostcommitTasksStartTask autocheckPostcommitTasksStartTask(
            SandboxClient sandboxClient) {
        return new AutocheckDegradationPostcommitTasksStartTask(dryRun, sandboxClient);
    }
}
