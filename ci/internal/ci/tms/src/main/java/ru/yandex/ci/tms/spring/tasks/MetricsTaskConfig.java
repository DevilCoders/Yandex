package ru.yandex.ci.tms.spring.tasks;

import java.time.Clock;

import org.apache.curator.framework.CuratorFramework;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.client.base.http.CallsMonitorSource;
import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.rm.RmClient;
import ru.yandex.ci.client.teamcity.TeamcityClient;
import ru.yandex.ci.client.testenv.TestenvClient;
import ru.yandex.ci.client.trendbox.TrendboxClient;
import ru.yandex.ci.client.tsum.TsumClient;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.spring.CommonConfig;
import ru.yandex.ci.engine.spring.clients.TestenvClientConfig;
import ru.yandex.ci.flow.spring.FlowZkConfig;
import ru.yandex.ci.flow.spring.ydb.YdbCiConfig;
import ru.yandex.ci.tms.metric.ci.CiSelfUsageMetricCronTask;
import ru.yandex.ci.tms.metric.ci.PotatoUsageMetricCronTask;
import ru.yandex.ci.tms.metric.ci.RmUsageMetricCronTask;
import ru.yandex.ci.tms.metric.ci.TeamcityUsageMetricTask;
import ru.yandex.ci.tms.metric.ci.TestenvUsageMetricCronTask;
import ru.yandex.ci.tms.metric.ci.TrendboxUsageMetricCronTask;
import ru.yandex.ci.tms.metric.ci.TsumUsageMetricCronTask;
import ru.yandex.ci.tms.spring.clients.RmClientConfig;
import ru.yandex.ci.tms.spring.clients.TeamcityClientConfig;
import ru.yandex.ci.tms.spring.clients.TsumClientConfig;
import ru.yandex.ci.tms.task.potato.PotatoClient;
import ru.yandex.ci.tms.task.potato.PotatoClientImpl;

@Configuration
@Import({
        CommonConfig.class,
        YdbCiConfig.class,
        FlowZkConfig.class,
        TestenvClientConfig.class,
        RmClientConfig.class,
        TeamcityClientConfig.class,
        TsumClientConfig.class
})
@Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
public class MetricsTaskConfig {

    @Bean
    public CiSelfUsageMetricCronTask ciSelfUsageMetricCronTask(CiMainDb db, CuratorFramework curator) {
        return new CiSelfUsageMetricCronTask(db, curator);
    }

    @Bean
    public RmUsageMetricCronTask rmUsageMetricCronTask(CiMainDb db, RmClient rmClient, CuratorFramework curator) {
        return new RmUsageMetricCronTask(db, rmClient, curator);
    }

    @Bean
    public TsumUsageMetricCronTask tsumUsageMetricCronTask(
            CiMainDb db,
            TsumClient tsumClient,
            CuratorFramework curator) {
        return new TsumUsageMetricCronTask(db, tsumClient, curator);
    }

    @Bean
    public TestenvUsageMetricCronTask testenvUsageMetricCronTask(
            CiMainDb db,
            TestenvClient testenvClient,
            CuratorFramework curator) {
        return new TestenvUsageMetricCronTask(db, testenvClient, curator);
    }


    @Bean
    public TeamcityUsageMetricTask teamcityUsageMetricTask(CiMainDb db,
                                                           TeamcityClient teamcityClient,
                                                           CuratorFramework curator) {
        return new TeamcityUsageMetricTask(db, teamcityClient, curator);
    }

    @Bean
    public PotatoClient potatoClient(CallsMonitorSource monitorSource) {
        return PotatoClientImpl.create(HttpClientProperties.ofEndpoint("https://potato.yandex-team.ru", monitorSource));
    }

    @Bean
    public PotatoUsageMetricCronTask potatoUsageMetricCronTask(CiMainDb db,
                                                               CuratorFramework curator,
                                                               Clock clock,
                                                               PotatoClient potatoClient) {
        return new PotatoUsageMetricCronTask(db, curator, clock, potatoClient);
    }

    @Bean
    public TrendboxClient trendboxClient() {
        return TrendboxClient.create(HttpClientProperties.ofEndpoint("https://beta-trendbox-ci.si.yandex-team.ru/"));
    }

    @Bean
    public TrendboxUsageMetricCronTask trendboxUsageMetricCronTask(CiMainDb db,
                                                                   CuratorFramework curator,
                                                                   TrendboxClient trendboxClient) {
        return new TrendboxUsageMetricCronTask(db, curator, trendboxClient);
    }
}
