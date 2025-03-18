package ru.yandex.ci.storage.tests;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CiBadgeEvents;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.test.TestUtils;

import static org.assertj.core.api.Assertions.assertThat;

public class BadgeEventsTest extends StorageTestsYdbTestBase {
    @Test
    public void startsIteration() {
        var registration = storageTester.register(registrar -> registrar.fullIteration().leftTask().rightTask());
        storageTester.executeAllOnetimeTasks();

        assertThat(nextEvent()).isEqualTo(eventResource("badge/check_created.pb"));
        assertThat(badgeEventsSender.noMoreEvents()).isTrue();

        storageTester.writeAndDeliver(registration, writer -> writer.toAll().finish());
        storageTester.executeAllOnetimeTasks();

        assertThat(nextEvent()).isEqualTo(eventResource("badge/iteration_finished.pb"));
        assertThat(nextEvent()).isEqualTo(eventResource("badge/check_finished.pb"));
        assertThat(badgeEventsSender.noMoreEvents()).isTrue();
    }

    @Test
    void restartIteration() {
        var registration = storageTester.register(registrar -> registrar
                .fullIteration()
                .leftTask(TASK_ID_LEFT)
                .rightTask(TASK_ID_RIGHT)
        );
        var iterationId = registration.getIteration().getId();
        var resultSuccess = exampleSuite(100L);
        var resultFailed = resultSuccess.toBuilder().setTestStatus(Common.TestStatus.TS_FAILED).build();

        storageTester.writeAndDeliver(registration, writer -> writer
                .toLeft().results(resultSuccess)
                .toRight().results(resultFailed)
                .to(TASK_ID_LEFT, TASK_ID_RIGHT).finish()
        );

        storageTester.executeAllOnetimeTasks();

        var secondRegistration = storageTester.register(
                registration,
                registrar -> registrar
                        // reuse iteration
                        .iteration(CheckIteration.IterationType.FULL, 2, Common.CheckTaskType.CTT_AUTOCHECK)
                        .leftTask("restart-task-id-left")
                        .rightTask("restart-task-id-right")
        );

        storageTester.writeAndDeliver(secondRegistration, writer -> writer
                .toAll()
                .results(resultSuccess)
                .finish()
        );

        storageTester.executeAllOnetimeTasks();

        var metaIteration = storageTester.getIteration(iterationId.toMetaId());
        assertThat(metaIteration.getStatus())
                .isEqualTo(Common.CheckStatus.COMPLETED_SUCCESS);

        assertThat(nextEvent()).isEqualTo(eventResource("badge/check_created.pb"));
        assertThat(nextEvent()).isEqualTo(eventResource("badge/meta_iteration_finished.pb"));
        assertThat(nextEvent()).isEqualTo(eventResource("badge/check_finished.pb"));
        assertThat(badgeEventsSender.noMoreEvents()).isTrue();
    }

    @Test
    void firstFail() {
        var registration = storageTester.register(registrar -> registrar
                .fullIteration()
                .leftTask(TASK_ID_LEFT)
                .rightTask(TASK_ID_RIGHT)
        );
        var iterationId = registration.getIteration().getId();

        var buildSuccess = exampleBuild(100L);
        var buildFailed = buildSuccess.toBuilder().setTestStatus(Common.TestStatus.TS_FAILED).build();

        storageTester.writeAndDeliver(registration, writer -> writer
                .toLeft().results(buildSuccess)
                .toRight().results(buildFailed)
                .to(TASK_ID_LEFT, TASK_ID_RIGHT)
        );

        storageTester.executeAllOnetimeTasks();

        assertThat(nextEvent()).isEqualTo(eventResource("badge/check_created.pb"));
        assertThat(nextEvent()).isEqualTo(eventResource("badge/first_fail.pb"));

        var styleSuccess = exampleTest(1L, 101L, Common.ResultType.RT_STYLE_CHECK);
        var styleFailed = styleSuccess.toBuilder().setTestStatus(Common.TestStatus.TS_FAILED).build();

        storageTester.writeAndDeliver(registration, writer -> writer
                .toLeft().results(styleSuccess)
                .toRight().results(styleFailed)
                .to(TASK_ID_LEFT, TASK_ID_RIGHT)
        );

        storageTester.executeAllOnetimeTasks();

        assertThat(badgeEventsSender.noMoreEvents())
                .withFailMessage("check that message wasn't sent again")
                .isTrue();

        storageTester.writeAndDeliver(registration, writer -> writer
                .toAll()
                .finish()
        );

        assertThat(storageTester.getIteration(iterationId).getStatus()).isEqualTo(Common.CheckStatus.COMPLETED_FAILED);
    }

    private CiBadgeEvents.Event eventResource(String resource) {
        return TestUtils.parseProtoText(resource, CiBadgeEvents.Event.class);
    }

    private CiBadgeEvents.Event nextEvent() {
        return badgeEventsSender.nextEvent();
    }
}
