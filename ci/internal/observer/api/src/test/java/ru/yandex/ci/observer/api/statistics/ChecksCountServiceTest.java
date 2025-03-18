package ru.yandex.ci.observer.api.statistics;

import java.time.Instant;
import java.time.temporal.ChronoUnit;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;
import java.util.stream.Stream;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.observer.api.ObserverApiYdbTestBase;
import ru.yandex.ci.observer.api.controllers.AuthorFilterDto;
import ru.yandex.ci.observer.api.controllers.IntervalDto;
import ru.yandex.ci.observer.core.db.model.check.CheckEntity;
import ru.yandex.ci.observer.core.db.model.check.ChecksCountStatement;
import ru.yandex.ci.storage.core.db.model.common.StorageRevision;

import static org.assertj.core.api.Assertions.assertThat;
import static ru.yandex.ci.storage.core.CheckOuterClass.CheckType.BRANCH_POST_COMMIT;
import static ru.yandex.ci.storage.core.CheckOuterClass.CheckType.BRANCH_PRE_COMMIT;
import static ru.yandex.ci.storage.core.CheckOuterClass.CheckType.TRUNK_POST_COMMIT;
import static ru.yandex.ci.storage.core.CheckOuterClass.CheckType.TRUNK_PRE_COMMIT;

class ChecksCountServiceTest extends ObserverApiYdbTestBase {

    private static final AtomicLong CHECK_ID_SEQUENCE = new AtomicLong();
    private static final AtomicInteger SVN_REVISION_SEQUENCE = new AtomicInteger();

    @Autowired
    ChecksCountService checksCountService;

    @Test
    void countYearInterval() {
        var year2020 = Instant.parse("2020-01-01T00:00:00Z");
        var year2021 = Instant.parse("2021-01-01T00:00:00Z");
        var year2022 = Instant.parse("2022-01-01T00:00:00Z");
        var year2023 = Instant.parse("2023-01-01T00:00:00Z");

        createChecksAtSpecificDate(
                addDays(year2020, 1),
                addDays(year2021, 1),
                addDays(year2022, 1),
                addDays(year2022, 2),
                addDays(year2023, 1)
        );

        var from = addDays(year2021, 10);
        var to = addDays(year2023, 100);
        clock.setTime(addDays(year2023, 20));

        assertThat(checksCountService.count(from, to, IntervalDto.YEAR, true, AuthorFilterDto.ALL))
                .containsExactly(
                        countByInterval(year2021, 1, 0, 0, 0),
                        countByInterval(year2022, 2, 0, 0, 0),
                        countByInterval(year2023, 1, 0, 0, 0)
                );
        assertThat(checksCountService.count(from, to, IntervalDto.YEAR, false, AuthorFilterDto.ALL))
                .containsExactly(
                        countByInterval(year2021, 1, 0, 0, 0),
                        countByInterval(year2022, 2, 0, 0, 0)
                );
    }

    @Test
    void countHourInterval() {
        createChecksAtSpecificDate(
                Instant.parse("2020-01-01T02:00:00Z"),
                Instant.parse("2020-01-01T02:30:00Z"),
                Instant.parse("2020-01-01T03:00:00Z"),
                Instant.parse("2020-01-01T03:30:00Z"),
                Instant.parse("2020-01-01T04:00:00Z")
        );

        var from = Instant.parse("2020-01-01T02:10:00Z");
        var to = Instant.parse("2020-01-01T03:40:00Z");
        assertThat(checksCountService.count(from, to, IntervalDto.HOUR, true, AuthorFilterDto.ALL))
                .containsExactly(
                        countByInterval(Instant.parse("2020-01-01T02:00:00Z"), 2, 0, 0, 0),
                        countByInterval(Instant.parse("2020-01-01T03:00:00Z"), 2, 0, 0, 0)
                );
        assertThat(checksCountService.count(from, to, IntervalDto.HOUR, false, AuthorFilterDto.ALL))
                .containsExactly(
                        countByInterval(Instant.parse("2020-01-01T02:00:00Z"), 2, 0, 0, 0)
                );
    }

    @Test
    void countWithAuthorFilter() {
        var date = Instant.parse("2020-06-01T00:00:00Z");
        var yearBegin = Instant.parse("2020-01-01T00:00:00Z");
        createChecksWithSpecificAuthor(date, "robot-a", "a-robot", "user-robot-user");

        var from = yearBegin;
        var to = Instant.parse("2021-01-01T00:00:00Z");
        assertThat(checksCountService.count(from, to, IntervalDto.YEAR, true, AuthorFilterDto.ALL))
                .containsExactly(countByInterval(yearBegin, 3, 0, 0, 0, 3));
        assertThat(checksCountService.count(from, to, IntervalDto.YEAR, true, AuthorFilterDto.EXCLUDE_ROBOTS))
                .containsExactly(countByInterval(yearBegin, 1, 0, 0, 0, 1));
        assertThat(checksCountService.count(from, to, IntervalDto.YEAR, true, AuthorFilterDto.ONLY_ROBOTS))
                .containsExactly(countByInterval(yearBegin, 2, 0, 0, 0, 2));
    }

    private void createChecksAtSpecificDate(Instant... dates) {
        db.currentOrTx(() -> Stream.of(dates)
                .map(this::arcCommit)
                .map(ChecksCountServiceTest::check)
                .forEach(db.checks()::save)
        );
    }

    private void createChecksWithSpecificAuthor(Instant date, String... authors) {
        db.currentOrTx(() -> Stream.of(authors)
                .map(author -> check(arcCommit(date))
                        .withAuthor(author)
                )
                .forEach(db.checks()::save)
        );
    }

    private ArcCommit arcCommit(Instant createTime) {
        return arcCommit(createTime, SVN_REVISION_SEQUENCE.incrementAndGet());
    }

    private ArcCommit arcCommit(Instant createTime, int svnRevision) {
        return ArcCommit.builder()
                .createTime(createTime)
                .id(ArcCommit.Id.of("r" + svnRevision)).svnRevision(svnRevision).build();
    }

    private static CheckEntity check(ArcCommit rightCommit) {
        return SAMPLE_CHECK
                .withId(CheckEntity.Id.of(CHECK_ID_SEQUENCE.incrementAndGet()))
                .withRight(StorageRevision.from(ArcBranch.trunk().asString(), rightCommit));
    }

    private static Instant addDays(Instant instant, int amountToAdd) {
        return instant.plus(amountToAdd, ChronoUnit.DAYS);
    }

    private static ChecksCountStatement.CountByInterval countByInterval(
            Instant year2021,
            int trunkPreCommit,
            int trunkPostCommit,
            int branchPostCommit,
            int branchPreCommit
    ) {
        return countByInterval(year2021, trunkPreCommit, trunkPostCommit, branchPostCommit, branchPreCommit, 1);
    }

    private static ChecksCountStatement.CountByInterval countByInterval(
            Instant year2021,
            int trunkPreCommit,
            int trunkPostCommit,
            int branchPostCommit,
            int branchPreCommit,
            int authorCount
    ) {
        return ChecksCountStatement.CountByInterval.of(
                year2021,
                Map.of(
                        TRUNK_PRE_COMMIT, trunkPreCommit, TRUNK_POST_COMMIT, trunkPostCommit,
                        BRANCH_PRE_COMMIT, branchPreCommit, BRANCH_POST_COMMIT, branchPostCommit
                ),
                trunkPreCommit + trunkPostCommit + branchPreCommit + branchPostCommit,
                authorCount
        );
    }

}
