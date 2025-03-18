package ru.yandex.ci.storage.tests;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.storage.core.Common;

import static org.assertj.core.api.Assertions.assertThat;

public class RestartsTest extends StorageTestsYdbTestBase {
    @Test
    public void plansRestartAndExecutes() {
        var registration = storageTester.register(registrar -> registrar.check()
                .fullIteration()
                .leftTask(TASK_ID_LEFT)
                .rightTask(TASK_ID_RIGHT)
                .leftTask(TASK_ID_LEFT_TWO)
                .rightTask(TASK_ID_RIGHT_TWO)
                .complete()
        );

        var result = exampleSuite(1001L);
        var resultTwo = exampleSuite(1002L);
        var resultThree = resultTwo.toBuilder()
                .setId(resultTwo.getId().toBuilder().setSuiteHid(1003L).setHid(1003L).setToolchain("other").build())
                .build();

        storageTester.writeAndDeliver(registration, writer -> writer.toLeft()
                .trace(DISTBUILD_STARTED)
                .results(result, resultTwo)
                .toRight()
                .results(
                        result.toBuilder()
                                .setTestStatus(Common.TestStatus.TS_FAILED)
                                .build(),
                        resultTwo.toBuilder()
                                .setTestStatus(Common.TestStatus.TS_FAILED)
                                .build()
                )
                .to(TASK_ID_LEFT_TWO)
                .results(resultThree)
                .to(TASK_ID_RIGHT_TWO)
                .results(
                        resultThree.toBuilder()
                                .setTestStatus(Common.TestStatus.TS_FAILED)
                                .build()
                )
                .toAll()
                .finish()
        );

        var check = storageTester.getCheck(registration.getCheck().getId());
        assertThat(check.getStatus()).isEqualTo(Common.CheckStatus.RUNNING);

        var iteration = storageTester.getIteration(registration.getIteration().getId());
        assertThat(iteration.getStatus()).isEqualTo(Common.CheckStatus.COMPLETED_FAILED);

        var restartIteration = storageTester.getIteration(iteration.getId().toIterationId(2));
        assertThat(restartIteration.getStatus()).isEqualTo(Common.CheckStatus.CREATED);

        var metaIteration = storageTester.getIteration(iteration.getId().toMetaId());
        assertThat(metaIteration.getStatus()).isEqualTo(Common.CheckStatus.RUNNING);

        storageTester.executeAllOnetimeTasks();

        var restarts = this.db.currentOrReadOnly(() -> this.db.suiteRestarts().findAll());
        assertThat(restarts).hasSize(6);

        restartIteration = storageTester.getIteration(iteration.getId().toIterationId(2));
        assertThat(restartIteration.getExpectedTasks()).hasSize(4);
    }
}
