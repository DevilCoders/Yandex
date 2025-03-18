package ru.yandex.ci.storage.shard.task;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.CommonTestBase;
import ru.yandex.ci.storage.core.Common.TestDiffType;
import ru.yandex.ci.storage.core.Common.TestStatus;
import ru.yandex.ci.storage.core.check.TaskResultComparer;

import static org.assertj.core.api.Assertions.assertThat;

public class TaskResultComparerTest extends CommonTestBase {
    @Test
    public void allTransitions() {
        for (var from : TestStatus.values()) {
            if (from.equals(TestStatus.UNRECOGNIZED)) {
                continue;
            }

            for (var to : TestStatus.values()) {
                if (to.equals(TestStatus.UNRECOGNIZED)) {
                    continue;
                }

                assertThat(
                        TaskResultComparer.compare(from, to, false)
                ).withFailMessage("%s to %s is null", from, to).isNotNull();
            }
        }
    }

    @Test
    public void testOkToOk() {
        Assertions.assertEquals(
                TestDiffType.TDT_PASSED, TaskResultComparer.compare(TestStatus.TS_OK, TestStatus.TS_OK, false)
        );
    }

    @Test
    public void testOkToFail() {
        Assertions.assertEquals(
                TestDiffType.TDT_FAILED_BROKEN,
                TaskResultComparer.compare(TestStatus.TS_OK, TestStatus.TS_FAILED, false)
        );
    }

    @Test
    public void testFailedToOk() {
        Assertions.assertEquals(
                TestDiffType.TDT_PASSED_FIXED,
                TaskResultComparer.compare(TestStatus.TS_FAILED, TestStatus.TS_OK, false)
        );
    }

    @Test
    public void testOkToSkipped() {
        Assertions.assertEquals(
                TestDiffType.TDT_SKIPPED,
                TaskResultComparer.compare(TestStatus.TS_OK, TestStatus.TS_SKIPPED, false)
        );
    }

    @Test
    public void testDiscoveredToSkipped() {
        Assertions.assertEquals(
                TestDiffType.TDT_SKIPPED,
                TaskResultComparer.compare(TestStatus.TS_DISCOVERED, TestStatus.TS_SKIPPED, false)
        );
    }
}
