package ru.yandex.ci.storage.tests;

import java.time.Instant;
import java.util.Comparator;
import java.util.stream.Collectors;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.ci.storage.tms.services.MuteDigestService;

import static org.assertj.core.api.Assertions.assertThat;

public class PostProcessorTest extends StorageTestsYdbTestBase {

    @Autowired
    private MuteDigestService muteDigestService;

    @Test
    public void mutesFlakyAndSendsNotification() {
        var now = Instant.parse("2015-01-02T10:00:00.000Z");
        clock.setTime(now);

        var registration = storageTester.register(registrar -> registrar.fullIteration().leftTask().rightTask());

        var suite = exampleSuite(100L).toBuilder().setOldId("oldId").build();
        storageTester.writeAndDeliver(
                registration,
                writer -> writer.toAll()
                        .results(suite, suite.toBuilder().setTestStatus(Common.TestStatus.TS_FAILED).build())
                        .finish()
        );

        var test = db.currentOrReadOnly(
                () -> db.tests().get(TestStatusEntity.Id.idInTrunk(CheckProtoMappers.toTestId(suite.getId())))
        );

        assertThat(test.isMuted()).isTrue();

        muteDigestService.execute(2);

        var last = db.currentOrTx(() -> db.keyValues().find(MuteDigestService.LAST_NOTIFICATION_KEY));
        assertThat(last).isPresent();
        assertThat(Instant.parse(last.get().getValue())).isEqualTo(now);

        var events = badgeEventsSender.getEvents().stream().toList();
        assertThat(events).hasSize(1);
    }

    @Test
    public void deletesTest() {
        var registration = storageTester.register(
                registrar -> registrar.check(r(1), r(2)).fullIteration().leftTask().rightTask()
        );

        var result = exampleSuite(100L);

        storageTester.writeAndDeliver(registration, writer -> writer.toLeft().results(result).toAll().finish());

        var test = db.currentOrReadOnly(
                () -> db.tests().get(TestStatusEntity.Id.idInTrunk(CheckProtoMappers.toTestId(result.getId())))
        );

        assertThat(test.getStatus()).isEqualTo(Common.TestStatus.TS_NONE);
        assertThat(test.getOldTestId()).isEqualTo(result.getOldId());

        var revisions = this.db.currentOrReadOnly(() -> this.db.testRevision().findAll()).stream()
                .sorted(Comparator.comparing(x -> x.getId().getRevision())).toList();
        assertThat(revisions).hasSize(2);
        assertThat(revisions.get(0).getId().getRevision()).isEqualTo(1L);
        assertThat(revisions.get(0).getRevisionCreated()).isEqualTo(r(1).getTimestamp());
        assertThat(revisions.get(1).getId().getRevision()).isEqualTo(2L);
        assertThat(revisions.get(1).getRevisionCreated()).isEqualTo(r(2).getTimestamp());
        assertThat(revisions.get(1).isChanged()).isTrue();
    }

    @Test
    public void ignoresOldStatus() {
        var registration = storageTester.register(
                registrar -> registrar.check(r(1), r(2)).fullIteration().leftTask().rightTask()
        );

        var secondRegistration = storageTester.register(
                registrar -> registrar.check(r(3), r(4)).fullIteration().leftTask().rightTask()
        );

        var result = exampleSuite(100L);
        var brokenResult = result.toBuilder().setTestStatus(Common.TestStatus.TS_FAILED).build();

        storageTester.writeAndDeliver(
                secondRegistration, writer -> writer.toAll().results(brokenResult).finish()
        );

        storageTester.writeAndDeliver(
                registration, writer -> writer.toAll().results(result).finish()
        );

        var test = db.currentOrReadOnly(
                () -> db.tests().get(TestStatusEntity.Id.idInTrunk(CheckProtoMappers.toTestId(result.getId())))
        );

        assertThat(test.getStatus()).isEqualTo(Common.TestStatus.TS_FAILED);
    }

    @Test
    public void ignoresSkipped() {
        var registration = storageTester.register(
                registrar -> registrar.check(r(1), r(2)).fullIteration().leftTask().rightTask()
        );

        var secondRegistration = storageTester.register(
                registrar -> registrar.check(r(3), r(4)).fullIteration().leftTask().rightTask()
        );

        var result = exampleSuite(100L);
        var skippedResult = result.toBuilder().setTestStatus(Common.TestStatus.TS_SKIPPED).build();

        storageTester.writeAndDeliver(
                registration, writer -> writer.toAll().results(result).finish()
        );

        storageTester.writeAndDeliver(
                secondRegistration, writer -> writer.toAll().results(skippedResult).finish()
        );

        var test = db.currentOrReadOnly(
                () -> db.tests().get(TestStatusEntity.Id.idInTrunk(CheckProtoMappers.toTestId(result.getId())))
        );

        assertThat(test.getStatus()).isEqualTo(Common.TestStatus.TS_OK);
    }

    @Test
    public void savesMetricsAndHistory() {
        var registration = storageTester.register(
                registrar -> registrar.fullIteration().leftTask().rightTask()
        );

        var result = exampleTest(1, 100, Common.ResultType.RT_TEST_MEDIUM).toBuilder()
                .putMetrics("agility", 1)
                .putMetrics("strength", 2)
                .putMetrics("stamina", 3)
                .putMetrics("intellect", 4)
                .build();

        var resultTwo = exampleTest(2, 100, Common.ResultType.RT_TEST_MEDIUM).toBuilder()
                .putMetrics("agility", 10)
                .putMetrics("strength", 20)
                .putMetrics("stamina", 30)
                .putMetrics("intellect", 40)
                .build();

        storageTester.writeAndDeliver(registration, writer -> writer.toAll().results(result, resultTwo).finish());

        /* todo tbd
        var mapping = this.db.currentOrReadOnly(() -> this.db.metricMapping().findAll());
        assertThat(mapping.stream().map(TestMetricMappingEntity::getNumber).collect(Collectors.toSet()))
                .contains(1, 2, 3, 4);

        var metrics = this.db.currentOrReadOnly(() -> this.db.testMetrics().findAll());
        assertThat(metrics).hasSize(8);
        */

        var history = this.db.currentOrReadOnly(() -> this.db.testLaunches().findAll());
        assertThat(history).hasSize(2);
        assertThat(history.get(0).getStatus()).isEqualTo(Common.TestStatus.TS_OK);
        assertThat(history.get(1).getStatus()).isEqualTo(Common.TestStatus.TS_OK);

        var revisions = this.db.currentOrReadOnly(() -> this.db.testRevision().findAll()).stream()
                .sorted(Comparator.comparing(x -> x.getId().getStatusId().getTestId()))
                .toList();

        assertThat(revisions).hasSize(2);
        assertThat(revisions.get(0).getId().getStatusId().getTestId()).isEqualTo(result.getId().getHid());
        assertThat(revisions.get(0).getStatus()).isEqualTo(result.getTestStatus());
        assertThat(revisions.get(1).getId().getStatusId().getTestId()).isEqualTo(resultTwo.getId().getHid());
        assertThat(revisions.get(1).getStatus()).isEqualTo(result.getTestStatus());
    }

    @Test
    public void marksRevisionAsChanged() {
        var registration = storageTester.register(
                registrar -> registrar.check(r(1), r(2)).fullIteration().leftTask().rightTask()
        );

        var result = exampleSuite(100);

        storageTester.writeAndDeliver(registration, writer -> writer.toAll().results(result).finish());

        var secondRegistration = storageTester.register(
                registrar -> registrar.check(r(3), r(4)).fullIteration().leftTask().rightTask()
        );

        var updatedResult = result.toBuilder().setTestStatus(Common.TestStatus.TS_FAILED).build();

        storageTester.writeAndDeliver(secondRegistration, writer -> writer.toAll().results(updatedResult).finish());

        var history = this.db.currentOrReadOnly(() -> this.db.testLaunches().findAll());
        assertThat(history).hasSize(4);

        var revisions = this.db.currentOrReadOnly(() -> this.db.testRevision().findAll()).stream()
                .collect(Collectors.toMap(x -> x.getId().getRevision(), x -> x));
        assertThat(revisions).hasSize(4);
        assertThat(revisions.get(1L).getStatus()).isEqualTo(result.getTestStatus());
        assertThat(revisions.get(2L).getStatus()).isEqualTo(result.getTestStatus());
        assertThat(revisions.get(3L).getStatus()).isEqualTo(updatedResult.getTestStatus());
        assertThat(revisions.get(4L).getStatus()).isEqualTo(updatedResult.getTestStatus());

        assertThat(revisions.get(3L).isChanged()).isTrue();
    }

    @Test
    public void countsEntities() {
        var registration = storageTester.register(
                registrar -> registrar.check(r(1), r(2)).fullIteration().leftTask().rightTask()
        );

        var result = exampleSuite(100);

        storageTester.writeAndDeliver(registration, writer -> writer.toAll().results(result).finish());

        assertThat(postProcessorStatistics.getNumberOfRegions()).isEqualTo(1);
        assertThat(postProcessorStatistics.getNumberOfBuckets()).isEqualTo(1);
        assertThat(postProcessorStatistics.getNumberOfRevisions()).isEqualTo(2);
    }
}
