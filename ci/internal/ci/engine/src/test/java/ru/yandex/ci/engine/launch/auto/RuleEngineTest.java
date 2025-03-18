package ru.yandex.ci.engine.launch.auto;

import java.time.Duration;
import java.time.Instant;
import java.time.ZoneOffset;
import java.time.ZonedDateTime;
import java.time.format.DateTimeFormatter;
import java.util.List;
import java.util.Optional;
import java.util.function.Supplier;

import com.fasterxml.jackson.core.type.TypeReference;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.Mock;
import org.mockito.junit.jupiter.MockitoExtension;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.a.model.auto.Conditions;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.launch.LaunchTable;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.core.time.DurationDeserializer;
import ru.yandex.ci.engine.timeline.CommitRangeService;
import ru.yandex.ci.flow.engine.runtime.calendar.TestWorkCalendarProvider;
import ru.yandex.ci.test.TestUtils;
import ru.yandex.ci.test.clock.OverridableClock;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.lenient;
import static org.mockito.Mockito.when;

@ExtendWith(MockitoExtension.class)
class RuleEngineTest {
    private static final CiProcessId PROCESS_ID = TestData.RELEASE_PROCESS_ID;
    private static final OrderedArcRevision REVISION = TestData.TRUNK_R8;

    private RuleEngine engine;

    @Mock
    private CommitRangeService commitRangeService;

    @Mock
    private CiMainDb db;

    @Mock
    private LaunchTable launchTable;

    private OverridableClock clock;

    @BeforeEach
    public void setUp() {
        clock = new OverridableClock();
        clock.stop();
        var scheduleCalculator = new ScheduleCalculator(new TestWorkCalendarProvider());
        engine = new RuleEngine(commitRangeService, clock, db, scheduleCalculator);

        //noinspection unchecked
        lenient().when(db.currentOrReadOnly(any(Supplier.class))).thenAnswer(invocation -> {
            Supplier<?> supplier = invocation.getArgument(0);
            return supplier.get();
        });
        lenient().when(db.launches()).thenReturn(launchTable);

        assertThat(clock.getZone()).isEqualTo(ZoneOffset.UTC);
    }

    @Test
    void empty() {
        Result test = engine.test(PROCESS_ID, REVISION, List.of());
        assertThat(test).isEqualTo(Result.launchRelease());
    }

    @Test
    void commitsNotEnough() {
        var when = parse("""
                - min-commits: 7
                """);

        mockCommitsToCapture(6);

        Result test = engine.test(PROCESS_ID, REVISION, when);
        assertThat(test).isEqualTo(Result.waitCommits());
    }

    @Test
    void commitsEnough() {
        var when = parse("""
                - min-commits: 7
                """);

        mockCommitsToCapture(7);

        Result test = engine.test(PROCESS_ID, REVISION, when);
        assertThat(test).isEqualTo(Result.launchRelease());
    }

    @Test
    void commitsEnoughMinimal() {
        var when = parse("""
                - min-commits: 7
                - min-commits: 6
                """);

        mockCommitsToCapture(6);

        Result test = engine.test(PROCESS_ID, REVISION, when);
        assertThat(test).isEqualTo(Result.launchRelease());
    }

    @Test
    void passedFirstRelease() {
        var when = parse("""
                - since-last-release: 1h
                """);

        Result test = engine.test(PROCESS_ID, REVISION, when);
        assertThat(test).isEqualTo(Result.launchRelease());
    }

    @Test
    void passedNotEnough() {
        var when = parse("""
                - since-last-release: 1h 2s
                """);

        mockLastCreatedReleaseAt(clock.instant());

        clock.plus(Duration.ofHours(1).plusSeconds(1));

        Result test = engine.test(PROCESS_ID, REVISION, when);

        assertThat(test).isEqualTo(Result.scheduleAt(clock.plusSeconds(1)));
    }

    @Test
    void passedEnough() {
        var when = parse("""
                - since-last-release: 1h
                """);

        mockLastCreatedReleaseAt(clock.instant());

        clock.plus(Duration.ofHours(1));
        Result result = engine.test(PROCESS_ID, REVISION, when);
        assertThat(result).isEqualTo(Result.launchRelease());
    }

    @Test
    void passedEnoughMinimal() {
        var when = parse("""
                - since-last-release: 2h
                - since-last-release: 50m
                - since-last-release: 2h 40m
                """);

        mockLastCreatedReleaseAt(clock.instant());
        clock.plus(Duration.ofMinutes(55));

        Result result = engine.test(PROCESS_ID, REVISION, when);
        assertThat(result).isEqualTo(Result.launchRelease());
    }

    @Test
    void windowContains() {
        var when = parse("""
                - schedule:
                    time: 10:00 - 19:30 Z
                    days: MON-FRI
                """);

        clock.setTime(dateTime("Tue, 3 Jun 2008 11:05:30 GMT"));

        Result result = engine.test(PROCESS_ID, REVISION, when);
        assertThat(result).isEqualTo(Result.launchRelease());
    }

    @Test
    void windowTooEarly() {
        var when = parse("""
                - schedule:
                    time: 10:00 - 19:30 Z
                    days: MON-FRI
                """);
        clock.setTime(dateTime("Tue, 3 Jun 2008 08:05:30 GMT"));

        Result result = engine.test(PROCESS_ID, REVISION, when);
        assertThat(result).isEqualTo(Result.scheduleAt(dateTime("Tue, 3 Jun 2008 10:00:00 GMT")));
    }

    @Test
    void windowTooLate() {
        var when = parse("""
                - schedule:
                    time: 10:00 - 19:30 Z
                    days: MON-FRI
                """);
        clock.setTime(dateTime("Tue, 3 Jun 2008 19:30:01 GMT"));

        Result result = engine.test(PROCESS_ID, REVISION, when);
        assertThat(result).isEqualTo(Result.scheduleAt(dateTime("Wed, 4 Jun 2008 10:00:00 GMT")));
    }

    @Test
    void notInDay() {
        var when = parse("""
                - schedule:
                    time: 10:00 - 19:30 Z
                    days: MON-FRI
                """);
        clock.setTime(dateTime("Sat, 7 Jun 2008 15:30:00 GMT"));

        Result result = engine.test(PROCESS_ID, REVISION, when);
        assertThat(result).isEqualTo(Result.scheduleAt(dateTime("Mon, 9 Jun 2008 10:00:00 GMT")));
    }

    @Test
    void tooLateNextWeek() {
        var when = parse("""
                - schedule:
                    time: 10:00 - 19:30 Z
                    days: MON-FRI
                """);
        clock.setTime(dateTime("Fri, 6 Jun 2008 19:32:00 GMT"));

        Result result = engine.test(PROCESS_ID, REVISION, when);
        assertThat(result).isEqualTo(Result.scheduleAt(dateTime("Mon, 9 Jun 2008 10:00:00 GMT")));
    }

    @Test
    void passedWaitMinimal() {
        var when = parse("""
                - since-last-release: 2h
                - since-last-release: 50m
                - since-last-release: 2h 40m
                """);

        mockLastCreatedReleaseAt(clock.instant());
        clock.plus(Duration.ofMinutes(45));

        Result result = engine.test(PROCESS_ID, REVISION, when);
        assertThat(result).isEqualTo(Result.scheduleAt(clock.plus(Duration.ofMinutes(5))));
    }

    @Test
    void passedInWindow() {
        var when = parse("""
                - since-last-release: 2h
                  schedule:
                    time: 10:00 - 19:30 Z
                    days: MON-FRI
                """);

        clock.setTime(dateTime("Fri, 6 Jun 2008 15:00:00 GMT"));
        mockLastCreatedReleaseAt(clock.instant().plus(duration("1h 30m").negated()));

        Result result = engine.test(PROCESS_ID, REVISION, when);
        assertThat(result).isEqualTo(Result.scheduleAt(clock.plus(Duration.ofMinutes(30))));
    }

    @Test
    void passedNotWindow() {
        var when = parse("""
                - since-last-release: 2h
                  schedule:
                    time: 10:00 - 19:30 Z
                    days: MON-FRI
                """);

        // через пол часа пройдет достаточно времени с последнего запуска
        // однако тогда уже выйдем за окно, поэтому проверим уже в следующее окно
        clock.setTime(dateTime("Fri, 6 Jun 2008 19:01:00 GMT"));
        mockLastCreatedReleaseAt(clock.instant().plus(duration("1h 30m").negated()));

        Result result = engine.test(PROCESS_ID, REVISION, when);
        assertThat(result).isEqualTo(Result.scheduleAt(dateTime("Mon, 9 Jun 2008 10:00:00 GMT")));
    }

    @Test
    void launchIfPossible() {
        var when = parse("""
                - since-last-release: 2h
                - schedule:
                    time: 10:00 - 19:30 Z
                    days: MON-FRI
                - min-commits: 3
                """);

        // через пол часа пройдет достаточно времени с последнего запуска
        // однако тогда уже выйдем за окно, поэтому проверим уже в следующее окно
        clock.setTime(dateTime("Fri, 6 Jun 2008 19:01:00 GMT"));
        mockLastCreatedReleaseAt(clock.instant().plus(duration("1h 30m").negated()));
        mockCommitsToCapture(4);

        Result test = engine.test(PROCESS_ID, REVISION, when);
        assertThat(test).isEqualTo(Result.launchRelease());
    }

    @Test
    void scheduleAtClosestTime() {
        var when = parse("""
                - since-last-release: 2h
                - schedule:
                    time: 10:00 - 19:30 Z
                    days: MON-FRI
                - min-commits: 99
                """);

        clock.setTime(dateTime("Fri, 6 Jun 2008 20:00:00 GMT"));
        mockLastCreatedReleaseAt(clock.instant().plus(duration("1h 30m").negated()));
        mockCommitsToCapture(4);

        Result result = engine.test(PROCESS_ID, REVISION, when);
        assertThat(result).isEqualTo(Result.scheduleAt(dateTime("Fri, 6 Jun 2008 20:30:00 GMT")));
    }

    private static Duration duration(String txt) {
        return new DurationDeserializer().convert(txt);
    }

    private static Instant dateTime(String formattedDateTime) {
        return ZonedDateTime.parse(formattedDateTime, DateTimeFormatter.RFC_1123_DATE_TIME)
                .toInstant();
    }

    private void mockLastCreatedReleaseAt(Instant created) {
        when(launchTable.getLastNotCancelledLaunch(PROCESS_ID))
                .thenReturn(Optional.of(TestData.launchBuilder()
                        .created(created)
                        .build())
                );
    }

    private void mockCommitsToCapture(int commitsCount) {
        when(commitRangeService.getCommitsToCapture(PROCESS_ID, ArcBranch.trunk(), REVISION))
                .thenReturn(rangeWithCommitsCount(commitsCount));
    }

    private CommitRangeService.Range rangeWithCommitsCount(int commitsCount) {
        return new CommitRangeService.Range(REVISION, null, commitsCount);
    }

    private static List<Conditions> parse(String yaml) {
        var typeRef = new TypeReference<List<Conditions>>() {
        };
        return TestUtils.parseYamlPartial(yaml, typeRef);
    }
}
