package ru.yandex.ci.tms.metric.ci;

import java.time.Clock;
import java.time.Duration;
import java.time.Instant;
import java.util.List;
import java.util.Map;
import java.util.function.Function;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import one.util.streamex.EntryStream;
import one.util.streamex.StreamEx;
import org.apache.curator.framework.CuratorFramework;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.potato.ConfigHealth;
import ru.yandex.ci.tms.task.potato.PotatoClient;
import ru.yandex.ci.tms.task.potato.client.Status;
import ru.yandex.lang.NonNullApi;

@NonNullApi
public class PotatoUsageMetricCronTask extends AbstractUsageMetricTask {
    private static final Logger log = LoggerFactory.getLogger(PotatoUsageMetricCronTask.class);

    private final CiMainDb db;
    private final Clock clock;
    private final PotatoClient potatoClient;

    private static final String JSON_TYPE = "json";
    private static final String YAML_TYPE = "yaml";

    private static final Duration ACTIVE_WINDOW = Duration.ofDays(20);


    private static final Pattern KEY_PATTERN = Pattern.compile(
            "rootLoader\\.parsing\\.(?<type>yaml|json)\\.(?<vcs>[^/]+)/(?<project>[^/]+)/(?<repo>.+)"
    );

    public PotatoUsageMetricCronTask(CiMainDb db, @Nullable CuratorFramework curator,
                                     Clock clock, PotatoClient potatoClient) {
        super(db, curator);
        this.db = db;
        this.clock = clock;
        this.potatoClient = potatoClient;
    }

    @Override
    public void computeMetric(MetricConsumer consumer) {
        refreshConfigs();
        var activeConfigs = getActive(ACTIVE_WINDOW);
        var counters = count(activeConfigs);


        consumer.addMetric(CiSystemsUsageMetrics.POTATO_PROJECTS, counters.projects);

        int bitbucketProjects = counters.bitbucketPublicProjects +
                counters.bitbucketEnterpriseProjects +
                counters.bitbucketBrowserProjects;
        int githubProjects = counters.githubPublicProjects + counters.githubEnterpriseProjects;
        consumer.addMetric(CiSystemsUsageMetrics.POTATO_ARCADIA_PROJECTS, counters.arcadiaProjects);
        consumer.addMetric(CiSystemsUsageMetrics.POTATO_BITBUCKET_PROJECTS, bitbucketProjects);
        consumer.addMetric(CiSystemsUsageMetrics.POTATO_GITHUB_PROJECTS, githubProjects);

        consumer.addMetric(CiSystemsUsageMetrics.POTATO_BITBUCKET_PUBLIC_PROJECTS, counters.bitbucketPublicProjects);
        consumer.addMetric(CiSystemsUsageMetrics.POTATO_BITBUCKET_ENTERPRISE_PROJECTS,
                counters.bitbucketEnterpriseProjects);
        consumer.addMetric(CiSystemsUsageMetrics.POTATO_BITBUCKET_BROWSER_PROJECTS, counters.bitbucketBrowserProjects);

        consumer.addMetric(CiSystemsUsageMetrics.POTATO_GITHUB_PUBLIC_PROJECTS, counters.githubPublicProjects);
        consumer.addMetric(CiSystemsUsageMetrics.POTATO_GITHUB_ENTERPRISE_PROJECTS, counters.githubEnterpriseProjects);

    }

    private static Counters count(List<ConfigHealth> configs) {
        var counters = new Counters();
        for (ConfigHealth config : configs) {
            counters.projects++;
            switch (config.getId().getVcs()) {
                // https://github.yandex-team.ru/data-ui/potato/blob/master/app/project-config/constants.js#L8
                case "arcanum" -> counters.arcadiaProjects++;
                case "github" -> counters.githubPublicProjects++;
                case "github-enterprise" -> counters.githubEnterpriseProjects++;
                case "bitbucket" -> counters.bitbucketPublicProjects++;
                case "bitbucket-enterprise" -> counters.bitbucketEnterpriseProjects++;
                case "bitbucket.browser-enterprise" -> counters.bitbucketBrowserProjects++;
                default -> log.warn("unknown vcs {}", config.getId().getVcs());
            }
        }
        return counters;
    }

    private List<ConfigHealth> getActive(Duration window) {
        return db.currentOrTx(() -> db.potatoConfigHealth().findSeenAfter(clock.instant().minus(window)));
    }

    private List<ConfigHealth> fetchConfigsFromPotato(String type) {
        Map<String, Status> response = potatoClient.healthCheck("rootLoader.parsing.%s.*".formatted(type));
        log.info("Fetched {} {} configs", response.size(), type);
        return EntryStream.of(response)
                .mapKeyValue(this::parseKey)
                .toList();
    }

    private void refreshConfigs() {
        List<ConfigHealth> fromPotato = fetchConfigsFromPotato(JSON_TYPE);
        fromPotato.addAll(fetchConfigsFromPotato(YAML_TYPE));

        log.info("Fetched total {} configs from potato", fromPotato.size());

        db.currentOrTx(() -> {
            var allConfigs = StreamEx.of(db.potatoConfigHealth().findAll())
                    .toMap(ConfigHealth::getId, Function.identity());


            log.info("Fetched {} configs for all time", allConfigs);

            var updated = StreamEx.of(fromPotato)
                    .filter(fromPotatoConfig -> {
                        var known = allConfigs.get(fromPotatoConfig.getId());
                        return known == null || fromPotatoConfig.getLastSeen().isAfter(known.getLastSeen());
                    })
                    .toImmutableList();

            log.info("{} of {} health are fresh", updated.size(), fromPotato.size());

            for (ConfigHealth updatedConfig : updated) {
                db.potatoConfigHealth().save(updatedConfig);
            }
        });
        log.info("Configs updated");
    }

    private ConfigHealth parseKey(String key, Status status) {
        Matcher matcher = KEY_PATTERN.matcher(key);
        Preconditions.checkState(matcher.matches(), "cannot parse %s", key);

        Instant lastSeen = status.getLastTs() != null
                ? Instant.ofEpochMilli(status.getLastTs())
                : clock.instant();

        return ConfigHealth.builder()
                .id(ConfigHealth.Id.builder()
                        .configType(matcher.group("type"))
                        .vcs(matcher.group("vcs"))
                        .project(matcher.group("project"))
                        .repo(matcher.group("repo"))
                        .build())
                .healthy(status.isHealthy())
                .lastSeen(lastSeen)
                .build();
    }

    private static class Counters {
        private int projects;
        private int arcadiaProjects;

        private int bitbucketPublicProjects;
        private int bitbucketEnterpriseProjects;
        private int bitbucketBrowserProjects;
        private int githubPublicProjects;
        private int githubEnterpriseProjects;
    }
}
