package ru.yandex.ci.engine.launch.auto;

import java.time.Clock;
import java.time.Instant;
import java.util.List;
import java.util.Optional;
import java.util.function.Supplier;

import javax.annotation.Nonnull;

import com.google.common.base.Suppliers;
import lombok.RequiredArgsConstructor;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.a.model.auto.Conditions;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.engine.timeline.CommitRangeService;
import ru.yandex.ci.engine.timeline.CommitRangeService.Range;

@Slf4j
@RequiredArgsConstructor
public class RuleEngine {
    private final CommitRangeService commitRangeService;
    private final Clock clock;
    private final CiMainDb db;
    private final ScheduleCalculator scheduleCalculator;

    public Result test(CiProcessId processId, OrderedArcRevision revision, List<Conditions> conditions) {
        Supplier<Range> range = Suppliers.memoize(() -> getFreeCommits(processId, revision));
        Supplier<Optional<Instant>> lastLaunch = Suppliers.memoize(
                () -> getLastLaunchTime(processId));

        var context = new RuleContext(range, lastLaunch, Instant.now(clock));

        log.info("Test {} at {} for {} conditions", processId, revision, conditions.size());
        var result = conditions.stream()
                .map(c -> test(c, context))
                .min(ResultComparator.instance())
                .orElse(Result.launchRelease());
        log.info("Result {}", result);
        return result;
    }

    private Range getFreeCommits(CiProcessId processId, OrderedArcRevision revision) {
        return db.currentOrReadOnly(() -> commitRangeService.getCommitsToCapture(
                processId, revision.getBranch(), revision));
    }

    private Result test(Conditions condition, RuleContext context) {
        log.info("Test {}", condition);
        if (condition.getMinCommits() != null
                && context.getCommitRange().getCount() < condition.getMinCommits()) {
            log.info("Too few commits {}", context.getCommitRange().getCount());
            return Result.waitCommits();
        }

        Instant scheduleAt = context.getNow();

        if (condition.getSinceLastRelease() != null) {
            Optional<Instant> lastLaunch = context.getLastLaunchCreated();
            if (lastLaunch.isPresent()) {
                Instant timeout = lastLaunch.get().plus(condition.getSinceLastRelease());
                if (timeout.isAfter(context.getNow())) {
                    scheduleAt = timeout;
                    log.info("Last release started at {}; schedule at {}", lastLaunch, scheduleAt);
                }
            } else {
                log.info("No last launch found");
            }
        }

        if (condition.getSchedule() != null) {
            scheduleAt = scheduleCalculator.nextWindow(condition.getSchedule(), scheduleAt).toInstant();
            log.info("Next window is {}", scheduleAt);
        }

        if (!scheduleAt.equals(context.getNow())) {
            return Result.scheduleAt(scheduleAt);
        }

        if (condition.getMinCommits() != null
                && condition.getMinCommits() == 0
                && condition.getSinceLastRelease() != null) {
            return Result.launchAndReschedule();
        }

        return Result.launchRelease();
    }

    private Optional<Instant> getLastLaunchTime(CiProcessId processId) {
        return db.currentOrReadOnly(() -> db.launches().getLastNotCancelledLaunch(processId)
                .map(Launch::getCreated));
    }

    @Value
    private static class RuleContext {
        @Nonnull
        Supplier<Range> commitRange; // Range between previous and current releases
        @Nonnull
        Supplier<Optional<Instant>> lastLaunchCreated; // `created` of previous release
        @Nonnull
        Instant now;

        public Range getCommitRange() {
            return commitRange.get();
        }

        public Optional<Instant> getLastLaunchCreated() {
            return lastLaunchCreated.get();
        }
    }
}

