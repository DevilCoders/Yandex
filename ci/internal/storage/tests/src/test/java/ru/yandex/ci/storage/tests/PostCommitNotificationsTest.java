package ru.yandex.ci.storage.tests;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.storage.core.CiBadgeEvents;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.constant.Trunk;
import ru.yandex.ci.storage.core.db.model.common.StorageRevision;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.ci.test.TestUtils;

import static org.assertj.core.api.Assertions.assertThat;

public class PostCommitNotificationsTest extends StorageTestsYdbTestBase {
    @Test
    public void sends() {
        var testsChanged = eventResource("badge/tests_changed.pb");
        var youBrokeTests = eventResource("badge/you_broke_tests.pb");
        var result = exampleSuite(1001L);

        var r1 = StorageRevision.from(Trunk.name(), arcService.getCommit(ArcRevision.of("r1")));
        var r2 = StorageRevision.from(Trunk.name(), arcService.getCommit(ArcRevision.of("r2")));

        var registration = storageTester.register(
                registrar -> registrar
                        .check(
                                check -> check
                                        .setLeftRevision(CheckProtoMappers.toProtoRevision(r1))
                                        .setRightRevision(CheckProtoMappers.toProtoRevision(r2))
                                        .setTestRestartsAllowed(false)
                        )
                        .fullIteration()
                        .leftTask(TASK_ID_LEFT)
                        .rightTask(TASK_ID_RIGHT)
        );

        storageTester.writeAndDeliver(registration, writer -> writer
                .toLeft().results(result)
                .toRight().results(result.toBuilder().setTestStatus(Common.TestStatus.TS_FAILED).build())
                .toAll()
                .finish()
        );

        storageTester.executeAllOnetimeTasks();

        var events = badgeEventsSender.getEvents().stream().toList();
        assertThat(events).hasSize(5);
        assertThat(events.get(1)).isEqualTo(testsChanged);
        assertThat(events.get(2)).isEqualTo(youBrokeTests);
    }

    private CiBadgeEvents.Event eventResource(String resource) {
        return TestUtils.parseProtoText(resource, CiBadgeEvents.Event.class);
    }
}
