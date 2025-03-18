package ru.yandex.ci.tms.spring.tasks;

import java.time.Clock;

import org.apache.curator.framework.CuratorFramework;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.juggler.JugglerPushClient;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.flow.spring.CommonServicesConfig;
import ru.yandex.ci.flow.spring.FlowZkConfig;
import ru.yandex.ci.flow.utils.UrlService;
import ru.yandex.ci.tms.client.SolomonClient;
import ru.yandex.ci.tms.spring.tasks.autocheck.degradation.AutocheckDegradationMonitoringsSourceConfig;
import ru.yandex.ci.tms.spring.tasks.autocheck.degradation.JugglerClientConfig;
import ru.yandex.ci.tms.task.LaunchMetricsExportCronTask;
import ru.yandex.ci.tms.task.release.CompleteReleaseToSolomonCronTask;
import ru.yandex.ci.tms.task.release.ReleaseStatusesToJugglerPushCronTask;

@Configuration
@Import({
        FlowZkConfig.class,
        CommonServicesConfig.class,
        JugglerClientConfig.class,
        AutocheckDegradationMonitoringsSourceConfig.class
})
public class CiTaskConfig {

    @Bean
    public ReleaseStatusesToJugglerPushCronTask releaseStatusesToJugglerPushCronTask(
            CuratorFramework curatorFramework,
            JugglerPushClient jugglerPushClient,
            CiMainDb db,
            UrlService urlService,
            @Value("${ci.releaseStatusesToJugglerPushCronTask.namespace}") String namespace
    ) {
        return new ReleaseStatusesToJugglerPushCronTask(curatorFramework, jugglerPushClient, db, namespace, urlService);
    }

    @Bean
    public LaunchMetricsExportCronTask launchMetricsExportCronTask(
            CuratorFramework curatorFramework,
            SolomonClient solomonClient,
            CiMainDb db,
            Clock clock,
            @Value("${ci.launchMetricsExportCronTask.project}") String project,
            @Value("${ci.launchMetricsExportCronTask.cluster}") String cluster,
            @Value("${ci.launchMetricsExportCronTask.service}") String service
    ) {
        var labels = new SolomonClient.RequiredMetricLabels(project, cluster, service);
        return new LaunchMetricsExportCronTask(curatorFramework, solomonClient, labels, db, clock);
    }

    @Bean
    public CompleteReleaseToSolomonCronTask completeReleaseToSolomonCronTask(
            CuratorFramework curatorFramework,
            SolomonClient solomonClient,
            CiMainDb db,
            Clock clock,
            @Value("${ci.completeReleaseToSolomonCronTask.project}") String project,
            @Value("${ci.completeReleaseToSolomonCronTask.cluster}") String cluster,
            @Value("${ci.completeReleaseToSolomonCronTask.service}") String service
    ) {
        var labels = new SolomonClient.RequiredMetricLabels(project, cluster, service);
        return new CompleteReleaseToSolomonCronTask(curatorFramework, solomonClient, labels, db, clock);
    }
}
