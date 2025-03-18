package ru.yandex.ci.storage.tests;

import java.time.Instant;
import java.util.List;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.storage.api.StorageFrontApi;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.archive.CheckArchiveService;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;

import static org.assertj.core.api.Assertions.assertThat;

public class ArchiveTest extends StorageTestsYdbTestBase {
    @Autowired
    private CheckArchiveService service;

    @Test
    public void plansAndExecuteArchive() {
        clock.setTime(Instant.parse("2020-01-01T00:00:00Z"));
        storageTester.register(storageTesterRegistrar -> storageTesterRegistrar.check(r(1), r(2)));

        clock.setTime(Instant.parse("2020-01-02T00:00:00Z"));
        storageTester.register(storageTesterRegistrar -> storageTesterRegistrar.check(r(2), r(3)));

        service.planChecksToArchive();
        assertThat(getInArchivingState()).isEmpty();

        clock.setTime(Instant.parse("2020-01-03T00:00:00Z"));
        service.planChecksToArchive();
        assertThat(getInArchivingState()).hasSize(1);

        clock.setTime(Instant.parse("2020-01-04T00:00:00Z"));
        service.planChecksToArchive();
        assertThat(getInArchivingState()).hasSize(2);

        storageTester.executeAllOnetimeTasks();
        assertThat(getInArchivingState()).hasSize(0);
    }

    private List<CheckEntity> getInArchivingState() {
        return db.currentOrReadOnly(
                () -> db.checks().getInArchiveState(CheckOuterClass.ArchiveState.AS_ARCHIVING, clock.instant(), 2)
        );
    }


    @Test
    public void archives() {
        var registration = storageTester.register(registrar -> registrar.fullIteration().leftTask().rightTask());
        var secondRegistration = storageTester.register(registrar -> registrar.fullIteration().leftTask().rightTask());

        var buildOne = exampleBuild(1L);
        var buildTwo = exampleBuild(2L);
        storageTester.writeAndDeliver(
                registration, writer -> writer
                        .toLeft()
                        .results(buildTwo)
                        .toRight()
                        .results(buildOne.toBuilder().setTestStatus(Common.TestStatus.TS_FAILED).build())
                        .toAll()
                        .results(buildOne)
                        .finish()
        );

        // restart
        storageTester.writeAndDeliver(
                secondRegistration, writer -> writer.toAll().results(buildTwo).finish()
        );

        var diffs = db.currentOrReadOnly(() -> db.testDiffs().findAll());
        assertThat(diffs).isNotEmpty();

        service.archive(registration.getCheck().getId());

        diffs = db.currentOrReadOnly(() -> db.testDiffs().findAll());
        var diffsByHash = db.currentOrReadOnly(() -> db.testDiffsByHash().findAll());
        var diffsBySuite = db.currentOrReadOnly(() -> db.testDiffsBySuite().findAll());
        var testResult = db.currentOrReadOnly(() -> db.testResults().findAll());
        var chunkAggregates = db.currentOrReadOnly(() -> db.chunkAggregates().findAll());
        var tasks = db.currentOrReadOnly(() -> db.checkTasks().findAll());
        var taskStatistics = db.currentOrReadOnly(() -> db.checkTaskStatistics().findAll());
        var suiteRestarts = db.currentOrReadOnly(() -> db.suiteRestarts().findAll());
        var largeTasks = db.currentOrReadOnly(() -> db.largeTasks().findAll());
        var importantDiffs = db.currentOrReadOnly(() -> db.importantTestDiffs().findAll());
        var textSearch = db.currentOrReadOnly(() -> db.checkTextSearch().findAll());

        assertThat(diffs).isEmpty();
        assertThat(diffsByHash).isEmpty();
        assertThat(diffsBySuite).isEmpty();
        assertThat(testResult).isEmpty();
        assertThat(chunkAggregates).isEmpty();
        assertThat(tasks).isEmpty();
        assertThat(taskStatistics).isEmpty();
        assertThat(suiteRestarts).isEmpty();
        assertThat(largeTasks).isEmpty();
        assertThat(textSearch).isEmpty();

        assertThat(importantDiffs).isNotEmpty();

        var suiteSearch = storageTester.frontApi().searchSuites(
                registration.getIteration().getId().toMetaId(),
                StorageFrontApi.SuiteSearch.newBuilder()
                        .setToolchain("@all")
                        .addResultType(Common.ResultType.RT_BUILD)
                        .setCategory(StorageFrontApi.CategoryFilter.CATEGORY_CHANGED)
                        .setStatusFilter(StorageFrontApi.StatusFilter.STATUS_FAILED)
                        .build()
        );

        assertThat(suiteSearch.getSuitesList())
                .withFailMessage("Must still search important diffs")
                .isNotEmpty();
    }
}
