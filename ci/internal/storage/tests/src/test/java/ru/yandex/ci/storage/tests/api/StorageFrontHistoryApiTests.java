package ru.yandex.ci.storage.tests.api;

import java.util.ArrayList;
import java.util.List;
import java.util.stream.Collectors;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.storage.api.StorageFrontHistoryApi;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.constant.Trunk;
import ru.yandex.ci.storage.core.db.model.common.StorageRevision;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.ci.storage.tests.StorageTestsYdbTestBase;
import ru.yandex.ci.storage.tests.TestsArcService;
import ru.yandex.ci.storage.tests.tester.StorageTesterRegistrar;

import static org.fest.assertions.Assertions.assertThat;
import static ru.yandex.ci.storage.core.Common.TestStatus.TS_NONE;


public class StorageFrontHistoryApiTests extends StorageTestsYdbTestBase {

    @Test
    public void getsTestInfo() {
        var registration = storageTester.register(registrar -> registrar.fullIteration().leftTask().rightTask());

        var suite = exampleSuite(100L).toBuilder().setOldId("oldId").build();
        storageTester.writeAndDeliver(
                registration,
                writer -> writer.toAll()
                        .results(suite, suite.toBuilder().setTestStatus(Common.TestStatus.TS_FAILED).build())
                        .finish()
        );

        var toolchains = storageTester.historyApi().getTestInfo(CheckProtoMappers.toTestId(suite.getId()))
                .getToolchainsList();

        assertThat(toolchains).hasSize(1);
        assertThat(toolchains.get(0).getName()).isEqualTo(suite.getName());
        assertThat(toolchains.get(0).getPath()).isEqualTo(suite.getPath());
        assertThat(toolchains.get(0).getRevision().getNumber()).isEqualTo(1L);
        assertThat(toolchains.get(0).getStatus()).isEqualTo(Common.TestStatus.TS_FLAKY);
        assertThat(toolchains.get(0).getLastMuteAction().getMuted()).isTrue();

        var testId = CheckProtoMappers.toTestStatusId(storageTester.historyApi().getTestIdByOldId("oldId"));
        assertThat(testId).isEqualTo(new TestStatusEntity.Id(Trunk.name(), CheckProtoMappers.toTestId(suite.getId())));
    }

    @Test
    public void getsLaunchForDeleted() {
        var registration = storageTester.register(
                registrar -> registrar.check(r(1), r(2)).fullIteration().leftTask().rightTask()
        );

        var build = exampleSuite(100L);
        storageTester.writeAndDeliver(
                registration, writer -> writer.toLeft().results(build).toAll().finish()
        );

        var testId = CheckProtoMappers.toTestId(build.getId());

        var history = storageTester.historyApi().getTestHistory(testId).getRevisionsList();
        assertThat(history).hasSize(1);

        var launches = storageTester.historyApi().getLaunches(testId, 2L).getLaunchesList();

        assertThat(launches).hasSize(1);
        assertThat(launches.get(0).getLastLaunch().getStatus()).isEqualTo(TS_NONE);
        assertThat(launches.get(0).getLastLaunch().getId().getIterationId().getCheckId()).isEqualTo(
                registration.getCheck().getId().getId().toString()
        );
    }

    @Test
    public void getsTestHistory() {
        var build = exampleBuild(1L);
        var testId = CheckProtoMappers.toTestId(build.getId()).toAllToolchainsId();

        var buildA = build.toBuilder().setId(build.getId().toBuilder().setToolchain("A").build()).build();
        var buildB = build.toBuilder().setId(build.getId().toBuilder().setToolchain("B").build()).build();
        var failedBuildA = buildA.toBuilder().setTestStatus(Common.TestStatus.TS_FAILED).build();

        var revisions = new ArrayList<StorageRevision>(10);
        revisions.add(null);
        for (int i = 1; i < 10; i++) {
            revisions.add(StorageRevision.from(Trunk.name(), arcService.getCommit(ArcRevision.of("r" + i))));
        }

        var registrations = getTestRegistrations(revisions);

        storageTester.writeAndDeliver(registrations.get(1), writer -> writer.toAll().results(buildA, buildB).finish());
        var history = storageTester.historyApi().getTestHistory(testId);
        assertThat(history.getRevisionsList()).hasSize(1);
        assertThat(history.getRevisionsList().get(0).getWrappedRevisionsBoundaries().getFrom()).isEqualTo(0);

        storageTester.writeAndDeliver(registrations.get(3), writer -> writer.toAll().results(buildA, buildB).finish());
        history = storageTester.historyApi().getTestHistory(testId);
        assertThat(history.getRevisionsList()).hasSize(1);
        var revision = history.getRevisionsList().get(0);
        assertThat(revision.getWrappedRevisionsBoundaries().getFrom()).isEqualTo(revisions.get(3).getRevisionNumber());
        assertThat(revision.getWrappedRevisionsBoundaries().getTo()).isEqualTo(0);

        storageTester.writeAndDeliver(
                registrations.get(2), writer -> writer.toAll().results(failedBuildA, buildB).finish()
        );

        history = storageTester.historyApi().getTestHistory(testId);
        assertThat(history.getRevisionsList()).hasSize(2);

        assertThat(history.getRevisionsList().get(0).getRevision().getNumber()).isEqualTo(3);
        assertThat(history.getRevisionsList().get(0).getWrappedRevisionsBoundaries().getFrom()).isEqualTo(0);
        assertThat(history.getRevisionsList().get(1).getRevision().getNumber()).isEqualTo(2);
        assertThat(history.getRevisionsList().get(1).getWrappedRevisionsBoundaries().getFrom()).isEqualTo(2);

        var launches = storageTester.historyApi().getLaunches(CheckProtoMappers.toTestId(buildA.getId()), 3)
                .getLaunchesList();
        assertThat(launches).hasSize(1);
        assertThat(launches.get(0).getNumberOfLaunches()).isEqualTo(1);
        assertThat(launches.get(0).getLastLaunch().getStatus()).isEqualTo(Common.TestStatus.TS_OK);

        storageTester.writeAndDeliver(
                registrations.get(4), writer -> writer.toAll().results(failedBuildA, buildB).finish()
        );

        storageTester.writeAndDeliver(
                registrations.get(5), writer -> writer.toAll().results(failedBuildA, buildB).finish()
        );

        storageTester.writeAndDeliver(
                registrations.get(6), writer -> writer.toAll().results(buildA, buildB).finish()
        );

        storageTester.writeAndDeliver(
                registrations.get(7), writer -> writer.toAll().results(failedBuildA, buildB).finish()
        );

        storageTester.writeAndDeliver(
                registrations.get(8), writer -> writer.toAll().results(buildA, buildB).finish()
        );

        history = storageTester.historyApi().getTestHistory(testId);
        assertThat(history.getRevisionsList().get(0).getRevision().getNumber()).isEqualTo(8);
        assertThat(history.getRevisionsList().get(1).getRevision().getNumber()).isEqualTo(7);

        history = storageTester.historyApi().getTestHistory(testId, history.getNext());
        assertThat(history.getRevisionsList().get(0).getRevision().getNumber()).isEqualTo(6);
        assertThat(history.getRevisionsList().get(1).getRevision().getNumber()).isEqualTo(4);

        history = storageTester.historyApi().getTestHistory(testId, history.getNext());
        assertThat(history.getRevisionsList().get(0).getRevision().getNumber()).isEqualTo(3);
        assertThat(history.getRevisionsList().get(1).getRevision().getNumber()).isEqualTo(2);
        assertThat(history.getNext().getFrom()).isEqualTo(0);

        history = storageTester.historyApi().getTestHistory(testId, history.getPrevious());
        assertThat(history.getRevisionsList().get(0).getRevision().getNumber()).isEqualTo(6);
        assertThat(history.getRevisionsList().get(1).getRevision().getNumber()).isEqualTo(4);
        var countRevisionResponse = storageTester.historyApi().countRevisions(
                testId,
                List.of(
                        history.getRevisionsList().get(0).getWrappedRevisionsBoundaries(),
                        StorageFrontHistoryApi.WrappedRevisionsBoundaries.newBuilder()
                                .setFrom(3)
                                .setTo(2)
                                .build()
                )
        );

        assertThat(countRevisionResponse.getBoundariesCount()).isEqualTo(2);
        assertThat(
                countRevisionResponse.getBoundariesList().stream().filter(x -> x.getBoundary().getFrom() == 6)
                        .findFirst().orElseThrow()
                        .getNumberOfRevisions()
        ).isEqualTo(1);

        var wrapped = storageTester.historyApi().getWrapped(
                testId,
                history.getRevisionsList().get(0).getWrappedRevisionsBoundaries()
        );

        assertThat(wrapped.getRevisionsList()).hasSize(1);
        var wrappedRevision = wrapped.getRevisionsList().get(0);
        assertThat(wrappedRevision.getWrappedRevisionsBoundaries().getFrom()).isEqualTo(0);
        assertThat(wrappedRevision.getWrappedRevisionsBoundaries().getTo()).isEqualTo(0);

        history = storageTester.historyApi().getTestHistory(testId, history.getPrevious());
        assertThat(history.getRevisionsList().get(0).getRevision().getNumber()).isEqualTo(8);
        assertThat(history.getRevisionsList().get(1).getRevision().getNumber()).isEqualTo(7);

        assertThat(history.getNext().getTo()).isEqualTo(0);
    }

    @Test
    public void fetchesWrapped() {
        var build = exampleBuild(1L);
        var testId = CheckProtoMappers.toTestId(build.getId()).toAllToolchainsId();

        var failedBuild = build.toBuilder().setTestStatus(Common.TestStatus.TS_FAILED).build();

        var revisions = new ArrayList<StorageRevision>(10);
        revisions.add(null);
        for (int i = 1; i < 10; i++) {
            revisions.add(StorageRevision.from(Trunk.name(), arcService.getCommit(ArcRevision.of("r" + i))));
        }

        var registrations = getTestRegistrations(revisions);

        storageTester.writeAndDeliver(registrations.get(1), writer -> writer.toAll().results(build).finish());
        storageTester.writeAndDeliver(registrations.get(2), writer -> writer.toAll().results(failedBuild).finish());
        storageTester.writeAndDeliver(registrations.get(3), writer -> writer.toAll().results(failedBuild).finish());
        storageTester.writeAndDeliver(registrations.get(4), writer -> writer.toAll().results(failedBuild).finish());
        storageTester.writeAndDeliver(registrations.get(5), writer -> writer.toAll().results(failedBuild).finish());
        storageTester.writeAndDeliver(registrations.get(6), writer -> writer.toAll().results(build).finish());


        var history = storageTester.historyApi().getTestHistory(testId);
        assertThat(history.getRevisionsList().get(0).getRevision().getNumber()).isEqualTo(6);
        assertThat(history.getRevisionsList().get(1).getRevision().getNumber()).isEqualTo(2);

        var wrapped = storageTester.historyApi().getWrapped(
                testId,
                history.getRevisionsList().get(0).getWrappedRevisionsBoundaries()
        );

        assertThat(wrapped.getRevisionsList()).hasSize(2);
        var wrappedRevision = wrapped.getRevisionsList().get(0);
        assertThat(wrappedRevision.getRevision().getNumber()).isEqualTo(5);
        assertThat(wrappedRevision.getWrappedRevisionsBoundaries().getFrom()).isEqualTo(0);
        assertThat(wrappedRevision.getWrappedRevisionsBoundaries().getTo()).isEqualTo(0);

        wrappedRevision = wrapped.getRevisionsList().get(1);
        assertThat(wrappedRevision.getRevision().getNumber()).isEqualTo(4);
        assertThat(wrappedRevision.getWrappedRevisionsBoundaries().getFrom()).isNotEqualTo(0);
        assertThat(wrappedRevision.getWrappedRevisionsBoundaries().getTo()).isNotEqualTo(0);

        wrapped = storageTester.historyApi().getWrapped(
                testId,
                wrapped.getRevisionsList().get(1).getWrappedRevisionsBoundaries()
        );

        assertThat(wrapped.getRevisionsList()).hasSize(1);
        wrappedRevision = wrapped.getRevisionsList().get(0);
        assertThat(wrappedRevision.getRevision().getNumber()).isEqualTo(3);
        assertThat(wrappedRevision.getWrappedRevisionsBoundaries().getFrom()).isEqualTo(0);
        assertThat(wrappedRevision.getWrappedRevisionsBoundaries().getTo()).isEqualTo(0);
    }

    private List<StorageTesterRegistrar.RegistrationResult> getTestRegistrations(List<StorageRevision> revisions) {
        var registrations = new ArrayList<StorageTesterRegistrar.RegistrationResult>(10);
        registrations.add(null);
        for (var i = 1; i < 9; i++) {
            var revisionNumber = i;
            registrations.add(
                    storageTester.register(
                            registrar -> registrar
                                    .check(
                                            revisions.get(revisionNumber),
                                            r("pr:42", "0396111a6031d8c0e0641e41a8627d6434973e13")
                                    )
                                    .fullIteration()
                                    .leftTask()
                    )
            );
        }
        return registrations;
    }

    @Test
    public void getLaunches() {
        var registration = storageTester.register(
                registrar -> registrar.check(r(1), pr(1, TestsArcService.PR_ONE_REVISION))
                        .fullIteration().leftTask().rightTask()
        );

        var secondRegistration = storageTester.register(
                registrar -> registrar.check(r(1), pr(2, TestsArcService.PR_TWO_REVISION))
                        .fullIteration().leftTask().rightTask()
        );

        var thirdRegistration = storageTester.register(
                registrar -> registrar.check(r(1), pr(3, TestsArcService.PR_THREE_REVISION))
                        .fullIteration().leftTask().rightTask()
        );

        var build = exampleBuild(1L);
        var failedBuild = build.toBuilder().setTestStatus(Common.TestStatus.TS_FAILED).build();
        var testId = CheckProtoMappers.toTestId(build.getId());
        storageTester.writeAndDeliver(registration, writer -> writer.toAll().results(build).finish());
        storageTester.writeAndDeliver(secondRegistration, writer -> writer.toAll().results(failedBuild).finish());
        storageTester.writeAndDeliver(thirdRegistration, writer -> writer.toAll().results(failedBuild).finish());

        var history = storageTester.historyApi().getTestHistory(testId);
        var launches = storageTester.historyApi().getLaunches(
                testId, history.getRevisionsList().get(0).getRevision().getNumber()
        ).getLaunchesList();

        assertThat(launches).hasSize(2);
        var launchesMap = launches.stream().collect(
                Collectors.groupingBy(StorageFrontHistoryApi.LaunchesByStatus::getStatus)
        );
        var okLaunches = launchesMap.get(Common.TestStatus.TS_OK);
        var failedLaunches = launchesMap.get(Common.TestStatus.TS_FAILED);

        assertThat(okLaunches).hasSize(1);
        assertThat(failedLaunches).hasSize(1);

        assertThat(failedLaunches.get(0).getLastLaunch().getId().getIterationId().getCheckId())
                .isEqualTo(thirdRegistration.getCheck().getId().getId().toString());
    }

    @Test
    public void getsMetrics() {
        var registration = storageTester.register(registrar -> registrar.fullIteration().leftTask().rightTask());

        var result = exampleTest(1, 100, Common.ResultType.RT_TEST_MEDIUM).toBuilder()
                .putMetrics("agility", 1)
                .putMetrics("strength", 2)
                .putMetrics("stamina", 3)
                .putMetrics("intellect", 4)
                .build();

        storageTester.writeAndDeliver(registration, writer -> writer.toAll().results(result).finish());

        /* todo tbd
        var testId = CheckProtoMappers.toTestId(result.getId());
        var metrics = storageTester.historyApi().getMetrics(testId);

        assertThat(metrics.getMetricsList().stream().map(StorageFrontHistoryApi.TestMetrics::getName).toList())
                .containsOnly("agility", "strength", "stamina", "intellect");

        var history = storageTester.historyApi().getTestMetricHistory(
                testId, metrics.getMetricsList().get(0).getMetricId()
        );
        assertThat(history.getPointsList()).hasSize(1);
         */
    }
}
