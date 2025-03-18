package ru.yandex.ci.storage.tests;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementDto;
import ru.yandex.ci.core.pr.ArcanumCheckType;

import static org.assertj.core.api.Assertions.assertThat;

public class PessimizationTest extends StorageTestsYdbTestBase {

    @Test
    public void setsMergeRequirement() {
        var registration = storageTester.register(registrar -> registrar.fullIteration().leftTask());
        storageTester.writeAndDeliver(
                writer -> writer.to(registration.getFirstLeft()).pessimize("bad check")
        );

        storageTester.executeAllOnetimeTasks();
        var pr = registration.getCheck().getPullRequestId().orElseThrow();
        var requirement = testArcanumClient.getMergeRequirement(pr, ArcanumCheckType.AUTOCHECK_PESSIMIZED);
        assertThat(requirement).isPresent();
        assertThat(requirement.get().getStatus()).isEqualTo(ArcanumMergeRequirementDto.Status.SUCCESS);
        assertThat(requirement.get().getMergeIntervalsUtc()).isNotEmpty();
        assertThat(requirement.get().getDescription()).isEqualTo("https://docs.yandex-team.ru/ci/autocheck/pessimized");
        var schedule = requirement.get().getMergeIntervalsUtc().get(0);
        assertThat(schedule.getFrom()).isEqualTo("16:00");
        assertThat(schedule.getTo()).isEqualTo("09:00");

        var required = testArcanumClient.required(pr, ArcanumCheckType.AUTOCHECK_PESSIMIZED);
        assertThat(required).isTrue();
    }
}
