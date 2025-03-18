package ru.yandex.ci.engine.launch.auto;

import java.time.Duration;
import java.time.Instant;
import java.time.ZoneOffset;
import java.time.ZonedDateTime;
import java.time.format.DateTimeFormatter;
import java.util.List;
import java.util.stream.Collectors;

import com.fasterxml.jackson.core.type.TypeReference;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.Mockito;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.mock.mockito.SpyBean;

import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.a.model.auto.Conditions;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.EngineTestBase;
import ru.yandex.ci.test.TestUtils;

import static java.util.Collections.emptyList;
import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.atLeastOnce;
import static org.mockito.Mockito.verify;
import static ru.yandex.ci.core.test.TestData.CI_USER;

public class AutoReleaseConditionalTest extends EngineTestBase {
    private static final CiProcessId PROCESS_ID = TestData.AUTO_RELEASE_SIMPLE_PROCESS_ID;

    @SpyBean
    private RuleEngine ruleEngine;

    @Autowired
    private DiscoveryProgressChecker discoveryProgressChecker;

    @SpyBean
    private ReleaseScheduler releaseScheduler;

    @BeforeEach
    public void setUp() {
        mockValidationSuccessful();

        assertThat(clock.getZone()).isEqualTo(ZoneOffset.UTC);
        clock.stop();
        clock.setTime(dateTime("Fri, 4 Jun 2021 20:30:00 GMT")); // See TestData.COMMIT_DATE, must be after that date

        discoveryToR2();
        delegateToken(PROCESS_ID.getPath());
    }

    @Test
    void commitLimit() {
        mockConditions("""
                - min-commits: 2
                """);

        autoReleaseService.processAutoReleaseQueue();
        assertThat(activeLaunchRevisions()).isEmpty();

        discoveryToR3();
        autoReleaseService.processAutoReleaseQueue();

        assertThat(activeLaunchRevisions()).containsExactly(TestData.TRUNK_R3);
    }

    @Test
    void autoSchedule() {
        mockConditions("""
                - min-commits: 0
                  since-last-release: 24h
                """);

        autoReleaseService.processAutoReleaseQueue();

        var initialLaunches = allLaunches();
        assertThat(initialLaunches.size()).isEqualTo(1);

        setLastLaunchState(initialLaunches, LaunchState.Status.SUCCESS);
        clock.plusHours(2);
        processAutoRelease();

        var firstAutoScheduledLaunches = allLaunches();
        assertThat(firstAutoScheduledLaunches.size()).isEqualTo(1);

        setLastLaunchState(firstAutoScheduledLaunches, LaunchState.Status.SUCCESS);
        clock.plusHours(23);
        processAutoRelease();

        var secondAutoScheduledLaunches = allLaunches();
        assertThat(secondAutoScheduledLaunches.size()).isEqualTo(2);
        assertThat(secondAutoScheduledLaunches.get(0).getCreated()).isEqualTo(clock.instant());

        setLastLaunchState(secondAutoScheduledLaunches, LaunchState.Status.CANCELED);
        clock.plusHours(2);
        processAutoRelease();

        var thirdAutoScheduledLaunches = allLaunches();
        assertThat(thirdAutoScheduledLaunches.size()).isEqualTo(3);
        assertThat(thirdAutoScheduledLaunches.get(0).getCreated()).isEqualTo(clock.instant());
    }

    @Test
    void autoScheduleWithLastFailed() {
        mockConditions("""
                - min-commits: 0
                  since-last-release: 24h
                """);

        autoReleaseService.processAutoReleaseQueue();

        var initialLaunches = allLaunches();
        assertThat(initialLaunches.size()).isEqualTo(1);

        setLastLaunchState(initialLaunches, LaunchState.Status.FAILURE);
        clock.plusHours(25);
        processAutoRelease();

        setLastLaunchState(initialLaunches, LaunchState.Status.SUCCESS);
        processAutoRelease();

        var scheduledLaunches = allLaunches();
        assertThat(scheduledLaunches.size()).isEqualTo(2);
        assertThat(scheduledLaunches.get(0).getCreated()).isEqualTo(clock.instant());
    }

    @Test
    void autoReleaseToggle() {
        mockConditions("""
                - min-commits: 0
                  since-last-release: 24h
                """);

        autoReleaseService.processAutoReleaseQueue();

        var initialLaunches = allLaunches();
        assertThat(initialLaunches.size()).isEqualTo(1);

        setLastLaunchState(initialLaunches, LaunchState.Status.SUCCESS);

        autoReleaseService.updateAutoReleaseState(PROCESS_ID, false, CI_USER, "");
        clock.plusHours(25);
        processAutoRelease();

        var firstAutoScheduledLaunches = allLaunches();
        assertThat(firstAutoScheduledLaunches.size()).isEqualTo(1);

        setLastLaunchState(firstAutoScheduledLaunches, LaunchState.Status.SUCCESS);

        autoReleaseService.updateAutoReleaseState(PROCESS_ID, true, CI_USER, "");
        clock.plusHours(25);
        processAutoRelease();

        var secondAutoScheduledLaunches = allLaunches();
        assertThat(secondAutoScheduledLaunches.size()).isEqualTo(2);
        assertThat(secondAutoScheduledLaunches.get(0).getCreated()).isEqualTo(clock.instant());

    }

    private void processAutoRelease() {
        executeBazingaTasks(DelayedAutoReleaseLaunchTask.class);
        autoReleaseService.processAutoReleaseQueue();
    }

    private void setLastLaunchState(List<Launch> launches, LaunchState.Status status) {
        var launch = launches.get(0);
        db.currentOrTx(() -> db.launches().save(
                launch.toBuilder()
                        .status(status)
                        .build()
        ));
    }

    @Test
    void sinceLastRelease() {
        mockConditions("""
                - since-last-release: 3h
                """);

        autoReleaseService.processAutoReleaseQueue();
        assertThat(activeLaunchRevisions()).containsExactly(TestData.TRUNK_R2);

        discoveryToR3();
        autoReleaseService.processAutoReleaseQueue();

        assertThat(activeLaunchRevisions()).containsExactly(TestData.TRUNK_R2);
        verify(releaseScheduler).schedule(PROCESS_ID, clock.instant().plus(Duration.ofHours(3)));

        clock.plusHours(4);
        executeBazingaTasks(DelayedAutoReleaseLaunchTask.class);
        autoReleaseService.processAutoReleaseQueue();

        assertThat(activeLaunchRevisions()).containsExactly(TestData.TRUNK_R3, TestData.TRUNK_R2);
    }

    @Test
    void scheduled() {
        clock.setTime(dateTime("Fri, 4 Jun 2021 20:30:00 GMT"));
        mockConditions("""
                - schedule:
                    time: 10 - 19:30 GMT
                    days: MON-FRI
                """);

        autoReleaseService.processAutoReleaseQueue();
        assertThat(activeLaunchRevisions()).isEmpty();
        verify(releaseScheduler).schedule(PROCESS_ID, dateTime("Mon, 7 Jun 2021 10:00:00 GMT"));

        discoveryToR3();
        autoReleaseService.processAutoReleaseQueue();

        assertThat(activeLaunchRevisions()).isEmpty();
        verify(releaseScheduler, atLeastOnce()).schedule(PROCESS_ID, dateTime("Mon, 7 Jun 2021 10:00:00 GMT"));

        clock.setTime(dateTime("Mon, 7 Jun 2021 10:01:00 GMT"));
        executeBazingaTasks(DelayedAutoReleaseLaunchTask.class);
        autoReleaseService.processAutoReleaseQueue();

        assertThat(activeLaunchRevisions()).containsExactly(TestData.TRUNK_R3);
    }

    @Test
    void scheduleChanged() {
        clock.setTime(dateTime("Fri, 4 Jun 2021 20:30:00 GMT"));
        mockConditions("""
                - schedule:
                    time: 10 - 19:30 GMT
                    days: MON-FRI
                """);

        autoReleaseService.processAutoReleaseQueue();
        assertThat(activeLaunchRevisions()).isEmpty();
        verify(releaseScheduler).schedule(PROCESS_ID, dateTime("Mon, 7 Jun 2021 10:00:00 GMT"));

        mockConditions("""
                - schedule:
                    time: 15-16 GMT
                    days: WED
                """);

        clock.setTime(dateTime("Mon, 7 Jun 2021 09:55:00 GMT"));
        discoveryToR3();
        autoReleaseService.processAutoReleaseQueue();
        verify(releaseScheduler).schedule(PROCESS_ID, dateTime("Wed, 9 Jun 2021 15:00:00 GMT"));
        executeBazingaTasks(DelayedAutoReleaseLaunchTask.class);

        assertThat(activeLaunchRevisions()).isEmpty();

        clock.setTime(dateTime("Wed, 9 Jun 2021 15:01:00 GMT"));
        executeBazingaTasks(DelayedAutoReleaseLaunchTask.class);
        autoReleaseService.processAutoReleaseQueue();

        assertThat(activeLaunchRevisions()).containsExactly(TestData.TRUNK_R3);
    }

    //region Helpers
    @Override
    protected void discovery(ArcCommit trunkCommit) {
        super.discovery(trunkCommit);
        discoveryProgressChecker.check();
    }

    private List<OrderedArcRevision> activeLaunchRevisions() {
        return db.currentOrReadOnly(() -> db.launches().getActiveLaunches(PROCESS_ID))
                .stream()
                .map(l -> l.getVcsInfo().getRevision())
                .collect(Collectors.toList());
    }

    private List<Launch> allLaunches() {
        return db.currentOrReadOnly(() -> db.launches().getLaunches(PROCESS_ID, 0, 42, false));
    }

    private static Instant dateTime(String formattedDateTime) {
        return ZonedDateTime.parse(formattedDateTime, DateTimeFormatter.RFC_1123_DATE_TIME)
                .toInstant();
    }

    private void mockConditions(String yaml) {
        List<Conditions> conditions = parse(yaml);
        Mockito.doAnswer(
                        invocation -> ruleEngine.test(PROCESS_ID, invocation.getArgument(1), conditions))
                .when(ruleEngine).test(eq(PROCESS_ID), any(), eq(emptyList()));
    }

    private static List<Conditions> parse(String yaml) {
        var typeRef = new TypeReference<List<Conditions>>() {
        };
        return TestUtils.parseYamlPartial(yaml, typeRef);
    }
    //endregion
}
