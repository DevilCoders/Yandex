package ru.yandex.ci.tools.flows;

import java.time.Instant;

import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.engine.launch.LaunchCleanupTask;
import ru.yandex.ci.engine.spring.tasks.EngineTasksConfig;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.spring.ydb.YdbCiConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;
import ru.yandex.commune.bazinga.impl.OnetimeUtils;
import ru.yandex.commune.bazinga.impl.TaskId;
import ru.yandex.misc.db.q.SqlLimits;

@Slf4j
@Configuration
@Import({
        YdbCiConfig.class,
        EngineTasksConfig.class
})
public class LoadBazingaJobs extends AbstractSpringBasedApp {

    @Autowired
    CiDb db;

    @Autowired
    LaunchCleanupTask launchCleanupTask;

    @Override
    public void run() {
        var jobs = db.currentOrReadOnly(() -> db.onetimeJobs().findOnetimeJobs(
                TaskId.from(launchCleanupTask.getClass()),
                Instant.ofEpochSecond(10000),
                SqlLimits.all()
        ));
        for (var job : jobs) {
            var params = (LaunchCleanupTask.Params)
                    OnetimeUtils.parseParameters(launchCleanupTask, job.getParameters());

            var launchId = params.getLaunchId();

            var launch = db.currentOrReadOnly(() -> db.launches().get(launchId));
            log.info("Launch {} has status {} (reason {})", launchId, launch.getStatus(), params.getCleanupReason());
        }
    }

    public static void main(String[] args) {
        startAndStopThisClass(args, Environment.STABLE);
    }
}
