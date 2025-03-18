package ru.yandex.ci.storage.tests;

import java.util.function.Function;
import java.util.stream.Collectors;
import java.util.stream.IntStream;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.constant.CheckStatusUtils;
import ru.yandex.ci.storage.core.db.model.check.SuspiciousAlert;
import ru.yandex.ci.storage.reader.check.suspicious.MoreTestsDeletedThanAddedRule;
import ru.yandex.ci.storage.reader.check.suspicious.NewOwnedFlakyRule;

import static org.assertj.core.api.Assertions.assertThat;

public class SuspiciousTest extends StorageTestsYdbTestBase {

    @Test
    public void manyTestsDeleted() {
        var registration = storageTester.register(registrar -> registrar.fullIteration().leftTask().rightTask());

        var tests = IntStream.range(0, 7).mapToObj(this::exampleSuite).toList();

        storageTester.writeAndDeliver(registration, writer -> writer
                .toLeft().results(tests)
                .toAll().finish()
        );

        var iteration = storageTester.getIteration(registration.getIteration().getId());

        assertThat(CheckStatusUtils.isCompleted(iteration.getStatus())).isTrue();

        assertThat(iteration.getSuspiciousAlerts()).isNotEmpty();
        assertThat(iteration.getSuspiciousAlerts())
                .extracting(SuspiciousAlert::getMessage)
                .anyMatch(message -> message.contains("7 tests deleted"));
    }

    @Test
    public void rightTimeouts() {
        var registration = storageTester.register(registrar -> registrar.fullIteration().leftTask().rightTask());

        var timeout = IntStream.range(1, 5).mapToObj(this::exampleBuild)
                .map(x -> x.toBuilder().setTestStatus(Common.TestStatus.TS_TIMEOUT).build())
                .toList();

        var ok = IntStream.range(5, 8).mapToObj(this::exampleBuild).toList();
        var allOk = IntStream.range(1, 8).mapToObj(this::exampleBuild).toList();

        storageTester.writeAndDeliver(registration, writer -> writer
                .toLeft()
                .results(allOk)
                .finish()
                .toRight()
                .results(timeout)
                .results(ok)
                .finish()
        );

        var iteration = storageTester.getIteration(registration.getIteration().getId());

        assertThat(CheckStatusUtils.isCompleted(iteration.getStatus())).isTrue();

        assertThat(iteration.getSuspiciousAlerts()).isNotEmpty();
        assertThat(iteration.getSuspiciousAlerts())
                .extracting(SuspiciousAlert::getMessage)
                .anyMatch(message -> message.contains("4 new timeouts"));
    }

    @Test
    public void metaIteration() {
        var registration = storageTester.register(registrar -> registrar.fullIteration().leftTask().rightTask());
        var secondRegistration = storageTester.register(registrar -> registrar.fullIteration().leftTask().rightTask());

        var tests = IntStream.range(0, 7).mapToObj(this::exampleSuite).toList();

        storageTester.writeAndDeliver(registration, writer -> writer
                .toLeft()
                .results(tests)
                .finish()
                .toRight()
                .finish()
        );

        var moreTests = IntStream.range(7, 10).mapToObj(this::exampleSuite).toList();

        storageTester.writeAndDeliver(secondRegistration, writer -> writer
                .toLeft().results(moreTests).finish()
                .toRight().finish()
        );

        var metaIteration = storageTester.getIteration(registration.getIteration().getId().toMetaId());

        assertThat(CheckStatusUtils.isCompleted(metaIteration.getStatus())).isTrue();

        var alerts = metaIteration.getSuspiciousAlerts().stream().collect(
                Collectors.toMap(SuspiciousAlert::getId, Function.identity())
        );

        assertThat(alerts).hasSize(1);
        assertThat(alerts.containsKey(MoreTestsDeletedThanAddedRule.class.getSimpleName())).isTrue();
        assertThat(alerts.get(MoreTestsDeletedThanAddedRule.class.getSimpleName()).getMessage())
                .contains("10 tests deleted");
    }

    @Test
    public void newOwnedFlaky() {
        var registration = storageTester.register(
                registrar -> registrar
                        .check(check -> check.setOwner("owner"))
                        .fullIteration().leftTask().rightTask()
        );

        var suiteResult = exampleSuite(100L);

        storageTester.writeAndDeliver(registration, writer -> writer
                .toLeft()
                .results(
                        suiteResult,
                        exampleMediumTest(1L, suiteResult.getId().getHid()).toBuilder()
                                .setTestStatus(Common.TestStatus.TS_OK)
                                .build()
                )
                .finish()
                .toRight()
                .results(
                        suiteResult,
                        exampleMediumTest(1L, suiteResult.getId().getHid()).toBuilder()
                                .setTestStatus(Common.TestStatus.TS_TIMEOUT)
                                .build(),
                        exampleMediumTest(2L, suiteResult.getId().getHid()).toBuilder()
                                .setTestStatus(Common.TestStatus.TS_FLAKY)
                                .build()
                )
                .finish()
        );

        var iteration = storageTester.getIteration(registration.getIteration().getId());

        assertThat(CheckStatusUtils.isCompleted(iteration.getStatus())).isTrue();

        assertThat(iteration.getSuspiciousAlerts()).isNotEmpty();
        assertThat(iteration.getSuspiciousAlerts())
                .extracting(SuspiciousAlert::getId)
                .anyMatch(id -> id.equals(NewOwnedFlakyRule.class.getSimpleName()));
    }

}
