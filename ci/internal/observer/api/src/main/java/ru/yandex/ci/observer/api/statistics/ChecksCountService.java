package ru.yandex.ci.observer.api.statistics;

import java.time.Clock;
import java.time.Instant;
import java.util.List;
import java.util.Objects;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.RequiredArgsConstructor;

import ru.yandex.ci.observer.api.controllers.AuthorFilterDto;
import ru.yandex.ci.observer.api.controllers.IntervalDto;
import ru.yandex.ci.observer.core.db.CiObserverDb;
import ru.yandex.ci.observer.core.db.model.check.ChecksCountStatement;

@RequiredArgsConstructor
public class ChecksCountService {

    @Nonnull
    private final CiObserverDb db;
    @Nonnull
    private final Clock clock;

    public List<ChecksCountStatement.CountByInterval> count(
            Instant from,
            @Nullable Instant to,
            IntervalDto intervalDto,
            boolean includeIncompleteInterval,
            AuthorFilterDto authorFilterDto
    ) {
        var intervalRightBound = getIntervalRightBoundOrNow(to);

        var interval = mapInterval(intervalDto);
        var authorFilter = mapAuthorFilter(authorFilterDto);

        return db.scan().run(() ->
                db.checks().count(from, intervalRightBound, interval, includeIncompleteInterval, authorFilter)
        );
    }

    private Instant getIntervalRightBoundOrNow(@Nullable Instant to) {
        var now = clock.instant();
        var intervalRightBound = Objects.requireNonNullElse(to, now);
        return now.isBefore(intervalRightBound) ? now : intervalRightBound;
    }

    private static ChecksCountStatement.Interval mapInterval(IntervalDto source) {
        return switch (source) {
            case HOUR -> ChecksCountStatement.Interval.HOUR;
            case DAY -> ChecksCountStatement.Interval.DAY;
            case WEEK -> ChecksCountStatement.Interval.WEEK;
            case MONTH -> ChecksCountStatement.Interval.MONTH;
            case QUARTER -> ChecksCountStatement.Interval.QUARTER;
            case YEAR -> ChecksCountStatement.Interval.YEAR;
        };
    }

    private static ChecksCountStatement.AuthorFilter mapAuthorFilter(AuthorFilterDto source) {
        return switch (source) {
            case ONLY_ROBOTS -> ChecksCountStatement.AuthorFilter.ONLY_ROBOTS;
            case EXCLUDE_ROBOTS -> ChecksCountStatement.AuthorFilter.EXCLUDE_ROBOTS;
            case ALL -> ChecksCountStatement.AuthorFilter.ALL;
        };
    }
}
