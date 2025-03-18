package ru.yandex.ci.tools.timeline;

import java.util.List;

import javax.annotation.Nullable;

import lombok.Value;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.boot.autoconfigure.jdbc.DataSourceAutoConfiguration;
import org.springframework.cache.annotation.Cacheable;
import org.springframework.cache.annotation.EnableCaching;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.db.model.ConfigState;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.project.ReleaseConfigState;
import ru.yandex.ci.core.timeline.Offset;
import ru.yandex.ci.core.timeline.TimelineBranchItem;
import ru.yandex.ci.core.timeline.TimelineItem;
import ru.yandex.ci.engine.spring.ConfigurationServiceConfig;
import ru.yandex.ci.engine.timeline.TimelineService;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Import({
        CacheConfig.class,
        ConfigurationServiceConfig.class
})
@EnableCaching
@SpringBootApplication(exclude = {
        DataSourceAutoConfiguration.class
})
@Configuration
public class CheckTimeline extends AbstractSpringBasedApp {

    @Autowired
    private CiMainDb db;

    @Autowired
    private TimelineService timelineService;

    @Autowired
    private CheckTimeline self;

    @Override
    protected void run() {
        var projects = self.getProjects();

        int total = 0;
        for (String project : projects) {
            List<ConfigState> configs = self.getConfigs(project).getValues();
            for (ConfigState config : configs) {
                for (ReleaseConfigState release : config.getReleases()) {
                    CiProcessId processId = CiProcessId.ofRelease(config.getConfigPath(), release.getReleaseId());
                    total += processTimeline(processId.asString());
                }
            }
        }
        log.info("Total issues: {}", total);
    }

    private static OrderedArcRevision getPreviousRevision(TimelineItem item) {
        if (item.getType() == TimelineItem.Type.LAUNCH) {
            return item.getLaunch().getVcsInfo().getPreviousRevision();
        } else {
            return item.getBranch().getItem().getVcsInfo().getPreviousRevision();
        }
    }

    private int processTimeline(String processId) {
        int count = 0;
        List<TimelineItem> timeline = self.getTimeline(processId).getItems();
        System.out.println("Timeline of " + processId + " has " + timeline.size() + " items");

        TimelineItem nextItem = null;
        for (TimelineItem currentItem : timeline) {

            if (nextItem == null) {
                nextItem = currentItem;
                continue;
            }

            if (!currentItem.getArcRevision().equals(getPreviousRevision(nextItem))) {
                fixItem(nextItem, currentItem.getArcRevision());
                count++;
            }

            nextItem = currentItem;
        }
        return count;
    }

    private void fixItem(TimelineItem item, OrderedArcRevision newPrevious) {
        db.currentOrTx(() -> {
            CiProcessId processId = item.getProcessId();
            String description = item.getType() == TimelineItem.Type.BRANCH
                    ? item.getBranch().getArcBranch().asString()
                    : item.getLaunch().getTitle();


            long prevNumber = newPrevious == null ? -1 : newPrevious.getNumber();
            int commitCount;
            if (item.getArcRevision().getNumber() == prevNumber) {
                commitCount = 0;
            } else {
                commitCount = db.discoveredCommit()
                        .count(processId, ArcBranch.trunk(), item.getArcRevision().getNumber(), prevNumber);
            }

            log.info("Should change {} --> {} [https://a.yandex-team.ru/ci/ci/releases/timeline?dir={}&id={}]" +
                            " for {} (new count {})",
                    getNumber(getPreviousRevision(item)), getNumber(newPrevious),
                    processId.getDir(), processId.getSubId(), description, commitCount);

            if (item.getType() == TimelineItem.Type.LAUNCH) {
                Launch launch = item.getLaunch();
                Launch updated = launch.toBuilder()
                        .vcsInfo(launch.getVcsInfo().toBuilder()
                                .previousRevision(newPrevious)
                                .commitCount(commitCount)
                                .build()
                        )
                        .build();
                log.info("launch updated {} --> {}", launch, updated);
                db.launches().save(updated);
            } else {
                TimelineBranchItem branch = item.getBranch().getItem();
                TimelineBranchItem updated = branch.toBuilder()
                        .vcsInfo(branch.getVcsInfo().toBuilder()
                                .previousRevision(newPrevious)
                                .trunkCommitCount(commitCount)
                                .build()
                        )
                        .build();
                log.info("branch updated {} --> {}", branch, updated);
                db.timelineBranchItems().save(updated);
            }
        });
    }

    @Nullable
    private static String getNumber(@Nullable OrderedArcRevision revision) {
        if (revision == null) {
            return null;
        }
        return String.valueOf(revision.getNumber());
    }

    @Cacheable("timeline")
    public Timeline getTimeline(String processId) {
        try {
            return new Timeline(db.readOnly(() -> timelineService.getTimeline(
                    CiProcessId.ofString(processId), ArcBranch.trunk(), Offset.EMPTY, 999))
            );
        } catch (IllegalStateException e) {
            log.error("Cannot load timeline for {}", processId, e);
            return new Timeline(List.of());
        }
    }

    @Value
    public static class Timeline {
        List<TimelineItem> items;
    }

    @Cacheable("configs")
    public Configs getConfigs(String project) {
        return new Configs(db.readOnly(() -> db.configStates().findByProject(project, true)));
    }

    @Value
    public static class Configs {
        List<ConfigState> values;
    }

    @Cacheable("projects")
    public List<String> getProjects() {
        return db.readOnly(() -> db.configStates()
                .listProjects(null, null, ConfigState.Status.hidden(), 0));
    }

    @Bean
    public List<String> cacheNames() {
        return List.of("projects", "configs", "timeline");
    }


    public static void main(String[] args) {
        startAndStopThisClass(args, Environment.STABLE);
    }
}
