package ru.yandex.ci.storage.tests.api;

import java.util.List;

import com.fasterxml.jackson.databind.node.NullNode;
import com.google.common.primitives.UnsignedLong;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;
import ru.yandex.ci.storage.core.db.model.check_task.LargeTaskEntity;
import ru.yandex.ci.storage.core.db.model.check_task.LargeTestInfo;
import ru.yandex.ci.storage.tests.StorageTestsYdbTestBase;

import static org.assertj.core.api.Assertions.assertThat;

public class StorageApiTests extends StorageTestsYdbTestBase {

    @Test
    public void comparesChecks() {
        var registration = storageTester.register(
                registrar -> registrar.check(r(1), r(2)).heavyIteration()
                        .leftTask("1")
                        .rightTask("2")
                        .leftTask("3")
                        .rightTask("4")
        );

        var secondRegistration = storageTester.register(
                registrar -> registrar.check(r(2), r(3)).heavyIteration()
                        .leftTask("1")
                        .rightTask("2")
                        .leftTask("3")
                        .rightTask("4")
        );

        var suite1Hid = 100L;
        var suite1 = exampleLargeTestSuite(suite1Hid);
        var test11 = exampleTest(10L, suite1Hid, Common.ResultType.RT_TEST_LARGE);
        var test12 = exampleTest(11L, suite1Hid, Common.ResultType.RT_TEST_LARGE);

        var suite2Hid = 200L;
        var suite2 = exampleLargeTestSuite(suite2Hid);
        var test21 = exampleTest(20L, suite2Hid, Common.ResultType.RT_TEST_LARGE);
        var test22 = exampleTest(21L, suite2Hid, Common.ResultType.RT_TEST_LARGE);
        storageTester.writeAndDeliver(
                registration,
                writer -> writer.to("1", "2").results(suite1, test11, test12)
        );
        storageTester.writeAndDeliver(
                registration,
                writer -> writer.to("3", "4").results(suite2, test21, test22)
        );

        var test12Broken = test12.toBuilder().setTestStatus(Common.TestStatus.TS_FAILED).build();
        storageTester.writeAndDeliver(
                secondRegistration,
                writer -> writer.to("1", "2").results(suite1, test11, test12Broken)
        );
        var test21Broken = test21.toBuilder().setTestStatus(Common.TestStatus.TS_FAILED).build();
        storageTester.writeAndDeliver(
                secondRegistration,
                writer -> writer.to("3", "4").results(suite2, test21Broken, test22)
        );

        // Saving out of order, but we need those ids
        var leftLargeId = saveLargeTaskEntity(
                registration.getTask("1"), registration.getTask("2"), suite1Hid);
        var rightLargeId = saveLargeTaskEntity(
                secondRegistration.getTask("1"), secondRegistration.getTask("2"), suite1Hid);


        // No finish on tasks
        var result = storageTester.api().compareChecks(leftLargeId, rightLargeId);
        assertThat(result.getCompareReady()).isFalse();

        storageTester.writeAndDeliver(
                registration,
                writer -> writer.to("1", "2").finish()
        );
        storageTester.writeAndDeliver(
                registration,
                writer -> writer.to("3", "4").finish()
        );

        // No finish on right yet
        result = storageTester.api().compareChecks(leftLargeId, rightLargeId);
        assertThat(result.getCompareReady()).isFalse();

        storageTester.writeAndDeliver(
                secondRegistration,
                writer -> writer.to("1", "2").finish()
        );

        // Don't expect finish for check or iteration, just check test types
        // Don't expect suite2 at all
        result = storageTester.api().compareChecks(leftLargeId, rightLargeId);
        assertThat(result.getCompareReady()).isTrue();

        var diffs = result.getDiffsList();
        assertThat(diffs).hasSize(1);

        var diff = diffs.get(0);
        assertThat(diff.getLeft()).isEqualTo(Common.TestStatus.TS_OK);
        assertThat(diff.getRight()).isEqualTo(Common.TestStatus.TS_FAILED);
        assertThat(diff.getTestId().getTestId()).isEqualTo("11");
        assertThat(diff.getDiffType()).isEqualTo(Common.TestDiffType.TDT_FAILED_BROKEN);

        // Try with different filters
        result = storageTester.api().compareChecks(
                leftLargeId,
                rightLargeId,
                Common.TestDiffType.TDT_FAILED,
                Common.TestDiffType.TDT_FAILED_BROKEN
        );
        assertThat(result.getCompareReady()).isTrue();

        diffs = result.getDiffsList();
        assertThat(diffs).hasSize(1);

        diff = diffs.get(0);
        assertThat(diff.getLeft()).isEqualTo(Common.TestStatus.TS_OK);
        assertThat(diff.getRight()).isEqualTo(Common.TestStatus.TS_FAILED);
        assertThat(diff.getTestId().getTestId()).isEqualTo("11");
        assertThat(diff.getDiffType()).isEqualTo(Common.TestDiffType.TDT_FAILED_BROKEN);

        // Try with different filters
        result = storageTester.api().compareChecks(
                leftLargeId,
                rightLargeId,
                Common.TestDiffType.TDT_FAILED
        );
        assertThat(result.getCompareReady()).isTrue();

        diffs = result.getDiffsList();
        assertThat(diffs).hasSize(0);
    }

    private LargeTaskEntity.Id saveLargeTaskEntity(
            CheckTaskEntity leftTask,
            CheckTaskEntity rightTask,
            long suiteId) {
        var largeTestInfo = new LargeTestInfo(
                "some-toolchain",
                List.of(),
                "some-test",
                "",
                UnsignedLong.fromLongBits(suiteId),
                NullNode.getInstance()
        );

        var id = new LargeTaskEntity.Id(leftTask.getId().getIterationId(), leftTask.getType(), 0);
        var largeTask = LargeTaskEntity.builder()
                .id(id)
                .target("some/test")
                .leftLargeTestInfo(largeTestInfo)
                .rightLargeTestInfo(largeTestInfo)
                .leftTaskId(leftTask.getId().getTaskId())
                .rightTaskId(rightTask.getId().getTaskId())
                .build();
        db.currentOrTx(() -> db.largeTasks().save(largeTask));
        return id;
    }
}
