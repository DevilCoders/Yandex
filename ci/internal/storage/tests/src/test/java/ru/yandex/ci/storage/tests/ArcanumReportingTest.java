package ru.yandex.ci.storage.tests;

import java.util.List;
import java.util.Map;
import java.util.function.Consumer;

import javax.annotation.Nullable;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.Arguments;
import org.junit.jupiter.params.provider.MethodSource;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementDto;
import ru.yandex.ci.core.pr.ArcanumCheckType;
import ru.yandex.ci.storage.api.StorageApi;
import ru.yandex.ci.storage.core.Actions;
import ru.yandex.ci.storage.core.Actions.TestTypeSizeFinished.Size;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckTaskStatus;
import ru.yandex.ci.storage.core.large.LargeStartService;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.ci.storage.tests.tester.StorageTesterRegistrar.RegistrationResult;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;

public class ArcanumReportingTest extends StorageTestsYdbTestBase {

    @Autowired
    private LargeStartService largeStartService;

    @Test
    public void reportsFailRestartsAndReportsSuccess() {
        var registration = storageTester.register(
                registrar -> registrar.fullIteration().leftTask(TASK_ID_LEFT).rightTask(TASK_ID_RIGHT)
        );

        var build = exampleBuild(1000L);
        var suite = exampleSuite(1001L);

        // build: OK - OK
        storageTester.writeAndDeliver(registration, writer -> writer.toAll().results(build));

        // suite: OK - FAILED
        storageTester.writeAndDeliver(
                registration,
                writer -> writer
                        .toLeft().results(suite)
                        .toRight().results(
                                suite.toBuilder()
                                        .setTestStatus(Common.TestStatus.TS_FAILED)
                                        .build()
                        )
        );

        storageTester.writeAndDeliver(
                registration,
                writer -> writer.toAll()
                        .testTypeFinish(Actions.TestType.CONFIGURE)
                        .testTypeFinish(Actions.TestType.BUILD)
        );

        // build completed

        storageTester.executeAllOnetimeTasks();

        var pr = registration.getCheck().getPullRequestId().orElseThrow();

        var buildStatus = testArcanumClient.status(pr, ArcanumCheckType.CI_BUILD);
        assertThat(buildStatus).isNotNull();
        assertThat(buildStatus).isEqualTo(ArcanumMergeRequirementDto.Status.SUCCESS);

        storageTester.writeAndDeliver(
                registration,
                writer -> writer.toAll()
                        .testTypeFinish(Actions.TestType.STYLE)
                        .testTypeSizeFinish(Size.SMALL)
                        .testTypeSizeFinish(Size.MEDIUM)
                        .testTypeSizeFinish(Size.LARGE)
                        .testTypeFinish(Actions.TestType.TEST)
        );

        // tests completed

        storageTester.executeAllOnetimeTasks();

        var testsStatus = testArcanumClient.status(pr, ArcanumCheckType.CI_TESTS);
        assertThat(testsStatus).isEqualTo(ArcanumMergeRequirementDto.Status.FAILURE);
        checkTaskStatuses(registration,
                CheckTaskStatus.NOT_REQUIRED,
                CheckTaskStatus.NOT_REQUIRED
        );

        storageTester.writeAndDeliver(registration, writer -> writer.toAll().finish());

        logbrokerService.deliverAllMessages();

        var iteration = storageTester.getIteration(registration.getIteration().getId());
        assertThat(iteration.getStatus()).isEqualTo(Common.CheckStatus.COMPLETED_FAILED);

        assertThat(iteration.getCheckTaskStatuses())
                .isEqualTo(Map.of(
                        Common.CheckTaskType.CTT_LARGE_TEST, CheckTaskStatus.NOT_REQUIRED,
                        Common.CheckTaskType.CTT_NATIVE_BUILD, CheckTaskStatus.NOT_REQUIRED
                ));

        // Restart

        storageTester.executeAllOnetimeTasks();

        var restartIteration = storageTester.getIteration(iteration.getId().toIterationId(2));

        assertThat(restartIteration.getExpectedTasks()).hasSize(2);

        assertThat(restartIteration.getStatus()).isEqualTo(Common.CheckStatus.CREATED);

        var secondRegistration = storageTester.register(
                registration,
                registrar -> registrar
                        // reuse iteration
                        .iteration(CheckIteration.IterationType.FULL, 2, Common.CheckTaskType.CTT_AUTOCHECK)
                        .leftTask("restart-task-id-left")
                        .rightTask("restart-task-id-right")
        );

        storageTester.writeAndDeliver(
                secondRegistration,
                writer -> writer.toAll()
                        .results(suite)
                        .testTypeFinish(Actions.TestType.CONFIGURE)
                        .testTypeFinish(Actions.TestType.BUILD)
        );

        // build completed

        storageTester.executeAllOnetimeTasks();

        buildStatus = testArcanumClient.status(pr, ArcanumCheckType.CI_BUILD);
        assertThat(buildStatus).isEqualTo(ArcanumMergeRequirementDto.Status.SUCCESS);

        storageTester.writeAndDeliver(
                secondRegistration,
                writer -> writer.toAll()
                        .testTypeFinish(Actions.TestType.STYLE)
                        .testTypeSizeFinish(Size.SMALL)
                        .testTypeSizeFinish(Size.MEDIUM)
                        .testTypeSizeFinish(Size.LARGE)
                        .testTypeFinish(Actions.TestType.TEST)
        );

        storageTester.writeAndDeliver(secondRegistration, writer -> writer.toAll().finish());

        // tests completed

        storageTester.executeAllOnetimeTasks();

        restartIteration = storageTester.getIteration(restartIteration.getId());
        assertThat(restartIteration.getRegisteredExpectedTasks()).hasSize(restartIteration.getExpectedTasks().size());
        assertThat(restartIteration.getStatus()).isEqualTo(Common.CheckStatus.COMPLETED_SUCCESS);

        var metaIteration = storageTester.getIteration(restartIteration.getId().toMetaId());
        assertThat(metaIteration.getStatus()).isEqualTo(Common.CheckStatus.COMPLETED_SUCCESS);

        testsStatus = testArcanumClient.status(pr, ArcanumCheckType.CI_TESTS);
        // flaky not counted as fail
        assertThat(testsStatus).isEqualTo(ArcanumMergeRequirementDto.Status.SUCCESS);

        assertThat(testArcanumClient.required(pr, ArcanumCheckType.CI_BUILD)).isNull();
        assertThat(testArcanumClient.required(pr, ArcanumCheckType.CI_TESTS)).isNull();
        assertThat(testArcanumClient.required(pr, ArcanumCheckType.CI_LARGE_TESTS)).isNull();

        assertThat(testArcanumClient.status(pr, ArcanumCheckType.CI_LARGE_TESTS))
                .isEqualTo(ArcanumMergeRequirementDto.Status.SKIPPED);
    }

    @Test
    public void reportsCancelFull() {
        var registration = storageTester.register(
                registrar -> registrar.fullIteration().leftTask(TASK_ID_LEFT)
        );

        storageTester.executeAllOnetimeTasks();

        var pr = registration.getCheck().getPullRequestId().orElseThrow();
        var largeTestsStatus = testArcanumClient.status(pr, ArcanumCheckType.CI_LARGE_TESTS);
        assertThat(largeTestsStatus).isEqualTo(ArcanumMergeRequirementDto.Status.SKIPPED);

        var required = testArcanumClient.required(pr, ArcanumCheckType.CI_LARGE_TESTS);
        assertThat(required).isNull();

        storageTester.api().cancel(CheckProtoMappers.toProtoIterationId(registration.getIteration().getId()));

        // build completed
        logbrokerService.deliverAllMessages();

        storageTester.executeAllOnetimeTasks();

        var buildStatus = testArcanumClient.status(pr, ArcanumCheckType.CI_BUILD);
        assertThat(buildStatus).isEqualTo(ArcanumMergeRequirementDto.Status.CANCELLED);

        var testsStatus = testArcanumClient.status(pr, ArcanumCheckType.CI_TESTS);
        assertThat(testsStatus).isEqualTo(ArcanumMergeRequirementDto.Status.CANCELLED);

        // unchanged
        largeTestsStatus = testArcanumClient.status(pr, ArcanumCheckType.CI_LARGE_TESTS);
        assertThat(largeTestsStatus).isEqualTo(ArcanumMergeRequirementDto.Status.SKIPPED);

        required = testArcanumClient.required(pr, ArcanumCheckType.CI_LARGE_TESTS);
        assertThat(required).isNull();
    }

    @Test
    public void reportsCancelFullWithLargeTests() {
        var registration = storageTester.register(
                registrar -> registrar.check(
                        checkBuilder -> checkBuilder.getInfoBuilder().addLargeAutostartBuilder()
                                .setTarget("a")
                                .setPath("b")
                ).fullIteration().leftTask(TASK_ID_LEFT)
        );

        storageTester.executeAllOnetimeTasks();

        var pr = registration.getCheck().getPullRequestId().orElseThrow();
        var largeTestsStatus = testArcanumClient.status(pr, ArcanumCheckType.CI_LARGE_TESTS);
        assertThat(largeTestsStatus).isEqualTo(ArcanumMergeRequirementDto.Status.PENDING);

        var required = testArcanumClient.required(pr, ArcanumCheckType.CI_LARGE_TESTS);
        assertThat(required).isTrue();

        storageTester.api().cancel(CheckProtoMappers.toProtoIterationId(registration.getIteration().getId()));

        // build completed
        logbrokerService.deliverAllMessages();

        storageTester.executeAllOnetimeTasks();

        var buildStatus = testArcanumClient.status(pr, ArcanumCheckType.CI_BUILD);
        assertThat(buildStatus).isEqualTo(ArcanumMergeRequirementDto.Status.CANCELLED);

        var testsStatus = testArcanumClient.status(pr, ArcanumCheckType.CI_TESTS);
        assertThat(testsStatus).isEqualTo(ArcanumMergeRequirementDto.Status.CANCELLED);

        // unchanged
        largeTestsStatus = testArcanumClient.status(pr, ArcanumCheckType.CI_LARGE_TESTS);
        assertThat(largeTestsStatus).isEqualTo(ArcanumMergeRequirementDto.Status.PENDING);

        required = testArcanumClient.required(pr, ArcanumCheckType.CI_LARGE_TESTS);
        assertThat(required).isTrue();
    }

    @Test
    public void reportsCancelHeavy() {
        var registration = storageTester.register(
                registrar -> registrar.heavyIteration().leftTask(TASK_ID_LEFT)
        );

        storageTester.executeAllOnetimeTasks();

        var pr = registration.getCheck().getPullRequestId().orElseThrow();
        var largeTestsStatus = testArcanumClient.status(pr, ArcanumCheckType.CI_LARGE_TESTS);
        assertThat(largeTestsStatus).isNull();

        var required = testArcanumClient.required(pr, ArcanumCheckType.CI_LARGE_TESTS);
        assertThat(required).isNull();

        storageTester.api().cancel(CheckProtoMappers.toProtoIterationId(registration.getIteration().getId()));

        // build completed
        logbrokerService.deliverAllMessages();

        storageTester.executeAllOnetimeTasks();

        var buildStatus = testArcanumClient.status(pr, ArcanumCheckType.CI_BUILD);
        assertThat(buildStatus).isNull();

        var testsStatus = testArcanumClient.status(pr, ArcanumCheckType.CI_TESTS);
        assertThat(testsStatus).isNull();

        largeTestsStatus = testArcanumClient.status(pr, ArcanumCheckType.CI_LARGE_TESTS);
        assertThat(largeTestsStatus).isEqualTo(ArcanumMergeRequirementDto.Status.CANCELLED);

        required = testArcanumClient.required(pr, ArcanumCheckType.CI_LARGE_TESTS);
        assertThat(required).isNull();
    }

    @Test
    public void reportsFatalError() {
        var registration = storageTester.register(registrar -> registrar.fullIteration().leftTask(TASK_ID_LEFT));

        var pr = registration.getCheck().getPullRequestId().orElseThrow();

        storageTester.writeAndDeliver(
                registration, writer -> writer.toAll().autocheckFatalError(exampleAutocheckFatalError())
        );
        storageTester.executeAllOnetimeTasks();

        var buildStatus = testArcanumClient.status(pr, ArcanumCheckType.CI_BUILD);
        assertThat(buildStatus).isEqualTo(ArcanumMergeRequirementDto.Status.ERROR);
        var iteration = storageTester.frontApi().getIteration(
                CheckProtoMappers.toProtoIterationId(registration.getIteration().getId())
        );
        assertThat(iteration.getInfo().getFatalErrorInfo().getSandboxTaskId()).isEqualTo("task-id");
    }

    @Test
    public void reports2IterationsCancelHeavy() {
        var registration = storageTester.register(
                registrar -> registrar.fullIteration().heavyIteration().leftTask(TASK_ID_LEFT)
        );

        storageTester.executeAllOnetimeTasks();

        var pr = registration.getCheck().getPullRequestId().orElseThrow();
        var largeTestsStatus = testArcanumClient.status(pr, ArcanumCheckType.CI_LARGE_TESTS);
        assertThat(largeTestsStatus).isEqualTo(ArcanumMergeRequirementDto.Status.SKIPPED);

        var required = testArcanumClient.required(pr, ArcanumCheckType.CI_LARGE_TESTS);
        assertThat(required).isNull();

        storageTester.api().cancel(CheckProtoMappers.toProtoIterationId(registration.getIteration().getId()));

        // build completed
        logbrokerService.deliverAllMessages();

        storageTester.executeAllOnetimeTasks();

        var buildStatus = testArcanumClient.status(pr, ArcanumCheckType.CI_BUILD);
        assertThat(buildStatus).isEqualTo(ArcanumMergeRequirementDto.Status.CANCELLED);

        var testsStatus = testArcanumClient.status(pr, ArcanumCheckType.CI_TESTS);
        assertThat(testsStatus).isEqualTo(ArcanumMergeRequirementDto.Status.CANCELLED);

        largeTestsStatus = testArcanumClient.status(pr, ArcanumCheckType.CI_LARGE_TESTS);
        assertThat(largeTestsStatus).isEqualTo(ArcanumMergeRequirementDto.Status.SKIPPED);

        required = testArcanumClient.required(pr, ArcanumCheckType.CI_LARGE_TESTS);
        assertThat(required).isNull();
    }

    @Test
    public void reports2IterationsCancelHeavyWithLargeTests() {
        var registration = storageTester.register(
                registrar -> registrar.check(
                        checkBuilder -> checkBuilder.getInfoBuilder().addLargeAutostartBuilder()
                                .setTarget("a")
                                .setPath("b/a.yaml")
                ).fullIteration().heavyIteration().leftTask(TASK_ID_LEFT)
        );

        storageTester.executeAllOnetimeTasks();

        var pr = registration.getCheck().getPullRequestId().orElseThrow();
        var largeTestsStatus = testArcanumClient.status(pr, ArcanumCheckType.CI_LARGE_TESTS);
        assertThat(largeTestsStatus).isEqualTo(ArcanumMergeRequirementDto.Status.PENDING);

        var required = testArcanumClient.required(pr, ArcanumCheckType.CI_LARGE_TESTS);
        assertThat(required).isTrue();

        storageTester.api().cancel(CheckProtoMappers.toProtoIterationId(registration.getIterations().get(0).getId()));
        storageTester.api().cancel(CheckProtoMappers.toProtoIterationId(registration.getIterations().get(1).getId()));

        // build completed
        logbrokerService.deliverAllMessages();

        storageTester.executeAllOnetimeTasks();

        var buildStatus = testArcanumClient.status(pr, ArcanumCheckType.CI_BUILD);
        assertThat(buildStatus).isEqualTo(ArcanumMergeRequirementDto.Status.CANCELLED);

        var testsStatus = testArcanumClient.status(pr, ArcanumCheckType.CI_TESTS);
        assertThat(testsStatus).isEqualTo(ArcanumMergeRequirementDto.Status.CANCELLED);

        largeTestsStatus = testArcanumClient.status(pr, ArcanumCheckType.CI_LARGE_TESTS);
        assertThat(largeTestsStatus).isEqualTo(ArcanumMergeRequirementDto.Status.CANCELLED);

        required = testArcanumClient.required(pr, ArcanumCheckType.CI_LARGE_TESTS);
        assertThat(required).isTrue();
    }

    @ParameterizedTest
    @MethodSource("checkMarkDiscoveredCommitTask")
    public void reportNoLargeTestsPending(
            CheckOuterClass.CheckType checkType,
            List<String> markCommits
    ) {
        var registration = register(checkType);

        storageTester.executeAllOnetimeTasks();

        checkLargePrStatus(registration, ArcanumMergeRequirementDto.Status.SKIPPED, null);
        checkTaskStatuses(registration,
                CheckTaskStatus.MAYBE_DISCOVERING,
                CheckTaskStatus.NOT_REQUIRED
        );

        checkMarkDiscoveredCommits(List.of());

        storageTester.writeAndDeliver(registration, writer ->
                writer.toAll().finish());
        storageTester.executeAllOnetimeTasks();

        checkLargePrStatus(registration, ArcanumMergeRequirementDto.Status.SKIPPED, null);
        checkTaskStatuses(registration,
                markCommits.isEmpty()
                        ? CheckTaskStatus.NOT_REQUIRED // Optimization
                        : CheckTaskStatus.COMPLETE, // Because OurGuys has 'user42'
                CheckTaskStatus.NOT_REQUIRED
        );

        checkMarkDiscoveredCommits(markCommits);
    }

    @ParameterizedTest
    @MethodSource("checkMarkDiscoveredCommitTask")
    public void reportLargeTestsPending(
            CheckOuterClass.CheckType checkType,
            List<String> markCommits
    ) {
        var registration = register(
                checkType,
                check -> check.getInfoBuilder().addLargeAutostart(largeAutostart())
        );

        storageTester.executeAllOnetimeTasks();

        checkLargePrStatus(registration, ArcanumMergeRequirementDto.Status.PENDING, true);
        checkTaskStatuses(registration,
                CheckTaskStatus.DISCOVERING,
                CheckTaskStatus.NOT_REQUIRED
        );

        checkMarkDiscoveredCommits(List.of());

        storageTester.writeAndDeliver(registration, writer ->
                writer.toAll().finish());
        storageTester.executeAllOnetimeTasks();

        checkLargePrStatus(registration, ArcanumMergeRequirementDto.Status.SKIPPED, true);
        checkTaskStatuses(registration,
                CheckTaskStatus.COMPLETE,
                CheckTaskStatus.NOT_REQUIRED
        );

        checkMarkDiscoveredCommits(markCommits);
    }

    @ParameterizedTest
    @MethodSource("checkMarkDiscoveredCommitTask")
    public void reportLargeTestsPendingWithSeparateStates(
            CheckOuterClass.CheckType checkType,
            List<String> markCommits
    ) {
        var registration = register(
                checkType,
                check -> check.getInfoBuilder().addLargeAutostart(largeAutostart())
        );

        storageTester.executeAllOnetimeTasks();

        checkLargePrStatus(registration, ArcanumMergeRequirementDto.Status.PENDING, true);
        checkTaskStatuses(registration,
                CheckTaskStatus.DISCOVERING,
                CheckTaskStatus.NOT_REQUIRED
        );

        checkMarkDiscoveredCommits(List.of());

        storageTester.writeAndDeliver(
                registration,
                writer -> writer.toAll()
                        .testTypeFinish(Actions.TestType.CONFIGURE)
                        .testTypeFinish(Actions.TestType.BUILD)
        );
        storageTester.executeAllOnetimeTasks();

        // Waiting for large tests discovering...
        checkLargePrStatus(registration, ArcanumMergeRequirementDto.Status.PENDING, true);
        checkTaskStatuses(registration,
                CheckTaskStatus.DISCOVERING,
                CheckTaskStatus.NOT_REQUIRED
        );

        checkMarkDiscoveredCommits(List.of());

        storageTester.writeAndDeliver(
                registration,
                writer -> writer.toAll()
                        .testTypeSizeFinish(Size.LARGE)
                        .testTypeFinish(Actions.TestType.TEST)
        );
        storageTester.executeAllOnetimeTasks();

        checkLargePrStatus(registration, ArcanumMergeRequirementDto.Status.SKIPPED, true);
        checkTaskStatuses(registration,
                CheckTaskStatus.COMPLETE,
                CheckTaskStatus.NOT_REQUIRED
        );

        checkMarkDiscoveredCommits(markCommits);
    }

    @ParameterizedTest
    @MethodSource("checkMarkDiscoveredCommitTask")
    public void reportNativeBuildsPending(CheckOuterClass.CheckType checkType, List<String> markCommits) {
        var registration = register(checkType, check -> check.getInfoBuilder().addNativeBuilds(nativeBuild()));

        storageTester.executeAllOnetimeTasks();

        checkNativePrStatus(registration, ArcanumMergeRequirementDto.Status.PENDING, true);
        checkTaskStatuses(registration, CheckTaskStatus.MAYBE_DISCOVERING, CheckTaskStatus.DISCOVERING);

        checkMarkDiscoveredCommits(List.of());

        storageTester.writeAndDeliver(registration, writer -> writer.toAll().finish());
        storageTester.executeAllOnetimeTasks();

        checkNativePrStatus(registration, ArcanumMergeRequirementDto.Status.SKIPPED, true);
        checkTaskStatuses(registration,
                markCommits.isEmpty()
                        ? CheckTaskStatus.NOT_REQUIRED // Optimization
                        : CheckTaskStatus.COMPLETE,
                CheckTaskStatus.COMPLETE
        );

        checkMarkDiscoveredCommits(markCommits);
    }

    @ParameterizedTest
    @MethodSource("checkMarkDiscoveredCommitTask")
    public void reportLargeTestsAndNativeBuildsPending(
            CheckOuterClass.CheckType checkType,
            List<String> markCommits
    ) {
        var registration = register(
                checkType,
                check -> check.getInfoBuilder()
                        .addLargeAutostart(largeAutostart())
                        .addNativeBuilds(nativeBuild())
        );

        storageTester.executeAllOnetimeTasks();

        checkLargePrStatus(registration, ArcanumMergeRequirementDto.Status.PENDING, true);
        checkTaskStatuses(registration,
                CheckTaskStatus.DISCOVERING,
                CheckTaskStatus.DISCOVERING
        );

        storageTester.writeAndDeliver(registration, writer ->
                writer.toAll().finish());
        storageTester.executeAllOnetimeTasks();

        checkLargePrStatus(registration, ArcanumMergeRequirementDto.Status.SKIPPED, true);
        checkTaskStatuses(registration,
                CheckTaskStatus.COMPLETE,
                CheckTaskStatus.COMPLETE
        );

        checkMarkDiscoveredCommits(markCommits);
    }


    @ParameterizedTest
    @MethodSource("checkMarkDiscoveredCommitTask")
    public void reportNoLargeTestsPendingMixedWithApiCallsEnableDisable(
            CheckOuterClass.CheckType checkType,
            List<String> markCommits
    ) {
        var registration = register(checkType);

        storageTester.executeAllOnetimeTasks();

        checkLargePrStatus(registration, ArcanumMergeRequirementDto.Status.SKIPPED, null);
        checkTaskStatuses(registration,
                CheckTaskStatus.MAYBE_DISCOVERING,
                CheckTaskStatus.NOT_REQUIRED
        );

        checkMarkDiscoveredCommits(List.of());

        storageTester.write(registration, writer ->
                writer.toAll().finish());

        scheduleLargeTestsManual(registration, true);
        checkLargePrStatus(registration, ArcanumMergeRequirementDto.Status.PENDING, true);

        scheduleLargeTestsManual(registration, true); // Second call does nothing
        checkLargePrStatus(registration, ArcanumMergeRequirementDto.Status.PENDING, true);

        scheduleLargeTestsManual(registration, false); // Reset pending state
        checkLargePrStatus(registration, ArcanumMergeRequirementDto.Status.SKIPPED, true);

        logbrokerService.deliverAllMessages();
        storageTester.executeAllOnetimeTasks();

        checkLargePrStatus(registration, ArcanumMergeRequirementDto.Status.SKIPPED, true);
        checkTaskStatuses(registration,
                CheckTaskStatus.NOT_REQUIRED, // Optimization
                CheckTaskStatus.NOT_REQUIRED
        );

        checkMarkDiscoveredCommits(markCommits);
    }

    @ParameterizedTest
    @MethodSource("checkMarkDiscoveredCommitTask")
    public void reportNoLargeTestsPendingMixedWithApiCalls(
            CheckOuterClass.CheckType checkType,
            List<String> markCommits
    ) {
        var registration = register(checkType);

        storageTester.executeAllOnetimeTasks();

        checkLargePrStatus(registration, ArcanumMergeRequirementDto.Status.SKIPPED, null);
        checkTaskStatuses(registration,
                CheckTaskStatus.MAYBE_DISCOVERING,
                CheckTaskStatus.NOT_REQUIRED
        );

        checkMarkDiscoveredCommits(List.of());

        storageTester.write(registration, writer ->
                writer.toAll().finish());

        scheduleLargeTestsManual(registration, true);
        checkLargePrStatus(registration, ArcanumMergeRequirementDto.Status.PENDING, true);

        logbrokerService.deliverAllMessages();
        storageTester.executeAllOnetimeTasks();

        checkLargePrStatus(registration, ArcanumMergeRequirementDto.Status.SKIPPED, true);
        checkTaskStatuses(registration,
                CheckTaskStatus.COMPLETE,
                CheckTaskStatus.NOT_REQUIRED
        );

        checkMarkDiscoveredCommits(markCommits);
    }

    @ParameterizedTest
    @MethodSource("checkMarkDiscoveredCommitTask")
    public void reportLargeTestsPendingMixedWithApiCallsEnableDisable(
            CheckOuterClass.CheckType checkType,
            List<String> markCommits
    ) {
        var registration = register(
                checkType,
                check -> check.getInfoBuilder().addLargeAutostart(largeAutostart())
        );

        storageTester.executeAllOnetimeTasks();

        checkLargePrStatus(registration, ArcanumMergeRequirementDto.Status.PENDING, true);
        checkTaskStatuses(registration,
                CheckTaskStatus.DISCOVERING,
                CheckTaskStatus.NOT_REQUIRED
        );

        checkMarkDiscoveredCommits(List.of());

        storageTester.write(registration, writer ->
                writer.toAll().finish());

        scheduleLargeTestsManual(registration, true);
        checkLargePrStatus(registration, ArcanumMergeRequirementDto.Status.PENDING, true);

        scheduleLargeTestsManual(registration, false); // No change in pending state
        checkLargePrStatus(registration, ArcanumMergeRequirementDto.Status.PENDING, true);

        logbrokerService.deliverAllMessages();
        storageTester.executeAllOnetimeTasks();

        checkLargePrStatus(registration, ArcanumMergeRequirementDto.Status.SKIPPED, true);
        checkTaskStatuses(registration,
                CheckTaskStatus.COMPLETE,
                CheckTaskStatus.NOT_REQUIRED
        );

        checkMarkDiscoveredCommits(markCommits);
    }

    @ParameterizedTest
    @MethodSource("checkMarkDiscoveredCommitTask")
    public void reportLargeTestsPendingMixedWithApiCalls(
            CheckOuterClass.CheckType checkType,
            List<String> markCommits
    ) {
        var registration = register(
                checkType,
                check -> check.getInfoBuilder().addLargeAutostart(largeAutostart())
        );

        storageTester.executeAllOnetimeTasks();

        checkLargePrStatus(registration, ArcanumMergeRequirementDto.Status.PENDING, true);
        checkTaskStatuses(registration,
                CheckTaskStatus.DISCOVERING,
                CheckTaskStatus.NOT_REQUIRED
        );

        checkMarkDiscoveredCommits(List.of());

        storageTester.write(registration, writer ->
                writer.toAll().finish());

        scheduleLargeTestsManual(registration, true);

        checkLargePrStatus(registration, ArcanumMergeRequirementDto.Status.PENDING, true);

        logbrokerService.deliverAllMessages();
        storageTester.executeAllOnetimeTasks();

        checkLargePrStatus(registration, ArcanumMergeRequirementDto.Status.SKIPPED, true);
        checkTaskStatuses(registration,
                CheckTaskStatus.COMPLETE,
                CheckTaskStatus.NOT_REQUIRED
        );

        checkMarkDiscoveredCommits(markCommits);
    }

    @ParameterizedTest
    @MethodSource("checkMarkDiscoveredCommitTask")
    public void reportNoLargeTestsPendingWithInvalidApiCalls(
            CheckOuterClass.CheckType checkType,
            List<String> markCommits
    ) {
        var registration = register(checkType);

        storageTester.executeAllOnetimeTasks();

        checkLargePrStatus(registration, ArcanumMergeRequirementDto.Status.SKIPPED, null);
        checkTaskStatuses(registration,
                CheckTaskStatus.MAYBE_DISCOVERING,
                CheckTaskStatus.NOT_REQUIRED
        );

        checkMarkDiscoveredCommits(List.of());

        storageTester.writeAndDeliver(registration, writer ->
                writer.toAll().finish());

        storageTester.executeAllOnetimeTasks();

        checkLargePrStatus(registration, ArcanumMergeRequirementDto.Status.SKIPPED, null);
        checkTaskStatuses(registration,
                markCommits.isEmpty()
                        ? CheckTaskStatus.NOT_REQUIRED
                        : CheckTaskStatus.COMPLETE,
                CheckTaskStatus.NOT_REQUIRED
        );

        checkMarkDiscoveredCommits(markCommits);

        // Not a problem
        scheduleLargeTestsManual(registration, false);

        // Now it's a problem
        assertThatThrownBy(() -> scheduleLargeTestsManual(registration, true))
                .hasMessage("Large tests already discovered, cannot schedule execution");
    }

    @ParameterizedTest
    @MethodSource("checkMarkDiscoveredCommitTask")
    public void reportLargeTestsPendingMixedWithInvalidApiCalls(
            CheckOuterClass.CheckType checkType,
            List<String> markCommits
    ) {
        var registration = register(
                checkType,
                check -> check.getInfoBuilder().addLargeAutostart(largeAutostart())
        );

        storageTester.executeAllOnetimeTasks();

        checkLargePrStatus(registration, ArcanumMergeRequirementDto.Status.PENDING, true);
        checkTaskStatuses(registration,
                CheckTaskStatus.DISCOVERING,
                CheckTaskStatus.NOT_REQUIRED
        );

        checkMarkDiscoveredCommits(List.of());

        storageTester.writeAndDeliver(registration, writer ->
                writer.toAll().finish());

        storageTester.executeAllOnetimeTasks();

        checkLargePrStatus(registration, ArcanumMergeRequirementDto.Status.SKIPPED, true);
        checkTaskStatuses(registration,
                CheckTaskStatus.COMPLETE,
                CheckTaskStatus.NOT_REQUIRED
        );

        checkMarkDiscoveredCommits(markCommits);

        // Not a problem
        scheduleLargeTestsManual(registration, false);

        // Now it's a problem
        assertThatThrownBy(() -> scheduleLargeTestsManual(registration, true))
                .hasMessage("Large tests already discovered, cannot schedule execution");
    }

    private CheckOuterClass.LargeAutostart largeAutostart() {
        return CheckOuterClass.LargeAutostart.newBuilder()
                .setTarget("a")
                .setPath("b/a.yaml")
                .build();
    }

    private CheckOuterClass.NativeBuild nativeBuild() {
        return CheckOuterClass.NativeBuild.newBuilder()
                .addTarget("a")
                .setPath("b/a.yaml")
                .build();
    }

    private RegistrationResult register(CheckOuterClass.CheckType checkType) {
        return register(checkType, check -> {
        });
    }

    private RegistrationResult register(
            CheckOuterClass.CheckType checkType,
            Consumer<StorageApi.RegisterCheckRequest.Builder> updateCheck
    ) {
        var result = register(check -> {
            updateCheck.accept(check);
            switch (checkType) {
                case TRUNK_PRE_COMMIT -> {
                    check.getLeftRevisionBuilder().setBranch("trunk");
                    check.getRightRevisionBuilder().setBranch("pr:42");
                }
                case TRUNK_POST_COMMIT -> {
                    check.getLeftRevisionBuilder().setBranch("trunk");
                    check.getRightRevisionBuilder()
                            .setBranch("trunk")
                            .setRevision("r2");
                }
                case BRANCH_PRE_COMMIT -> {
                    check.getLeftRevisionBuilder().setBranch("release/ci-1");
                    check.getRightRevisionBuilder().setBranch("pr:42");

                    // Make sure to set delegated config
                    check.getInfoBuilder()
                            .setDefaultLargeConfig(CheckOuterClass.LargeConfig.newBuilder()
                                    .setPath("config")
                                    .setRevision(Common.OrderedRevision.newBuilder()
                                            .setRevision("r1")
                                            .build())
                                    .build());
                }
                case BRANCH_POST_COMMIT -> {
                    check.getLeftRevisionBuilder().setBranch("release/ci-1");
                    check.getRightRevisionBuilder().setBranch("release/ci-1");
                }
                default -> throw new IllegalArgumentException();
            }
        });
        assertThat(result.getCheck().getType())
                .isEqualTo(checkType);
        return result;
    }

    private RegistrationResult register(Consumer<StorageApi.RegisterCheckRequest.Builder> updateCheck) {
        return storageTester.register(registrar -> registrar
                .check(updateCheck)
                .fullIteration()
                .leftTask()
                .rightTask()
        );
    }

    private void checkLargePrStatus(
            RegistrationResult registration,
            ArcanumMergeRequirementDto.Status expectStatus,
            @Nullable Boolean expectRequired
    ) {
        checkPrStatus(ArcanumCheckType.CI_LARGE_TESTS, registration, expectStatus, expectRequired);
    }

    private void checkNativePrStatus(
            RegistrationResult registration,
            ArcanumMergeRequirementDto.Status expectStatus,
            @Nullable Boolean expectRequired
    ) {
        checkPrStatus(ArcanumCheckType.CI_BUILD_NATIVE, registration, expectStatus, expectRequired);
    }

    private void checkPrStatus(
            ArcanumCheckType checkType,
            RegistrationResult registration,
            ArcanumMergeRequirementDto.Status expectStatus,
            @Nullable Boolean expectRequired
    ) {
        if (!LargeStartService.isPrecommit(registration.getCheck().getType())) {
            assertThat(registration.getCheck().getPullRequestId()).isEmpty();
            return;
        }

        var pr = registration.getCheck().getPullRequestId().orElseThrow();

        var status = testArcanumClient.status(pr, checkType);
        assertThat(status).isEqualTo(expectStatus);

        var required = testArcanumClient.required(pr, checkType);
        assertThat(required).isEqualTo(expectRequired);
    }

    private void checkTaskStatuses(
            RegistrationResult registration,
            CheckTaskStatus largeTestStatus,
            CheckTaskStatus nativeBuildStatus) {
        var iteration = storageTester.getIteration(registration.getIteration().getId());
        assertThat(iteration.getCheckTaskStatuses())
                .isEqualTo(Map.of(
                        Common.CheckTaskType.CTT_LARGE_TEST, largeTestStatus,
                        Common.CheckTaskType.CTT_NATIVE_BUILD, nativeBuildStatus
                ));
    }

    private void checkMarkDiscoveredCommits(List<String> expectCommits) {
        var actualMarkCommitRequests = testCiClient.getMarkDiscoveredCommitRequests().stream()
                .map(request -> request.getRevision().getCommitId())
                .toList();
        assertThat(actualMarkCommitRequests).isEqualTo(expectCommits);
    }

    private void scheduleLargeTestsManual(RegistrationResult registration, boolean runLargeTestsAfterDiscovery) {
        largeStartService.scheduleLargeTestsManual(
                registration.getCheck().getId(),
                runLargeTestsAfterDiscovery,
                "username");
        storageTester.executeAllOnetimeTasks();
    }


    static List<Arguments> checkMarkDiscoveredCommitTask() {
        return List.of(
                Arguments.of(CheckOuterClass.CheckType.TRUNK_PRE_COMMIT, List.of()),
                Arguments.of(CheckOuterClass.CheckType.TRUNK_POST_COMMIT, List.of("r2")),
                Arguments.of(CheckOuterClass.CheckType.BRANCH_PRE_COMMIT, List.of()),
                Arguments.of(CheckOuterClass.CheckType.BRANCH_POST_COMMIT, List.of())
        );
    }
}
