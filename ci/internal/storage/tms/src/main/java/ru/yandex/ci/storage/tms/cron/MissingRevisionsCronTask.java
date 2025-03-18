package ru.yandex.ci.storage.tms.cron;

import java.util.Collection;
import java.util.Objects;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import lombok.extern.slf4j.Slf4j;
import org.joda.time.Duration;

import yandex.cloud.repository.kikimr.yql.YqlLimit;

import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.CommitNotFoundException;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.constant.Trunk;
import ru.yandex.ci.storage.core.db.model.revision.MissingRevisionEntity;
import ru.yandex.ci.storage.core.db.model.revision.RevisionEntity;
import ru.yandex.commune.bazinga.scheduler.CronTask;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.commune.bazinga.scheduler.schedule.Schedule;
import ru.yandex.commune.bazinga.scheduler.schedule.SchedulePeriodic;

@Slf4j
public class MissingRevisionsCronTask extends CronTask {
    private final CiStorageDb db;
    private final ArcService arcService;
    private final Schedule schedule;

    public MissingRevisionsCronTask(CiStorageDb db, ArcService arcService) {
        this.db = db;
        this.arcService = arcService;
        this.schedule = new SchedulePeriodic(1, TimeUnit.DAYS);
    }

    @Override
    public Schedule cronExpression() {
        return this.schedule;
    }

    @Override
    public Duration timeout() {
        return Duration.standardDays(1);
    }

    @Override
    public void execute(ExecutionContext executionContext) {
        while (true) {
            var revisions = db.currentOrReadOnly(() -> db.missingRevisions().find(YqlLimit.top(1000))).stream()
                    .map(MissingRevisionEntity::getId)
                    .collect(Collectors.toSet());

            if (revisions.isEmpty()) {
                return;
            }

            process(revisions);

            db.currentOrTx(() -> db.missingRevisions().delete(revisions));
        }
    }

    private void process(Collection<MissingRevisionEntity.Id> revisions) {
        log.info(
                "Loading revisions: {}",
                revisions.stream()
                        .map(MissingRevisionEntity.Id::toString)
                        .collect(Collectors.joining(", "))
        );

        var commits = revisions.stream()
                .map(this::load)
                .filter(Objects::nonNull)
                .map(x -> new RevisionEntity(Trunk.name(), x))
                .toList();

        db.currentOrTx(() -> db.revisions().bulkUpsert(commits, Integer.MAX_VALUE));
    }

    @Nullable
    private ArcCommit load(MissingRevisionEntity.Id x) {
        try {
            return arcService.getCommit(ArcRevision.of("r" + x.getNumber()));
        } catch (CommitNotFoundException e) {
            log.info("Commit not found " + x);
            return null;
        }
    }
}
