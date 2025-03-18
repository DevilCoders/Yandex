package ru.yandex.ci.tms.test;

import java.util.List;

import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.MethodSource;

import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchState.Status;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.flow.YavDelegationException;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResourceContainer;

import static org.assertj.core.api.Assertions.assertThat;

public class EntireTestConditional extends AbstractEntireTest {

    @ParameterizedTest
    @MethodSource
    void woodcutterConditional(CiProcessId processId) throws YavDelegationException, InterruptedException {
        processCommits(TestData.TRUNK_COMMIT_2);

        engineTester.delegateToken(processId.getPath());

        Launch launch = launch(processId, TestData.TRUNK_R2);

        var finalLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.SUCCESS);
        var flowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(finalLaunch.getLaunchId()));

        assertThat(flowLaunch.getJobState("sawmill-1").isConditionalSkip())
                .isTrue();
        assertThat(flowLaunch.getJobState("sawmill-2").isConditionalSkip())
                .isFalse();

        StoredResourceContainer resources =
                flowTestQueries.getProducedResources(FlowLaunchId.of(launch.getLaunchId()), "furniture-factory");

        var furnitures = getFurniture(resources);

        var user = TestData.CI_USER;
        var linden = "бревно из дерева Липа, которую заказал %s".formatted(user);
        var birch = "бревно из дерева Береза, которую срубили по просьбе %s".formatted(user);

        var sawmill1 = "Лесопилка обычная, пилит 2 бревна";

        var furniture = "Шкаф из 3 досок, полученных из материала";
        assertThat(furnitures).isEqualTo(
                List.of("%s '%s', произведенного [%s]".formatted(furniture, birch, sawmill1),
                        "%s '%s', произведенного [%s]".formatted(furniture, linden, sawmill1)
                ));

        var boards = getBoards(resources);
        assertThat(boards).isEmpty();
    }

    static List<CiProcessId> woodcutterConditional() {
        return List.of(
                TestData.SAWMILL_RELEASE_CONDITIONAL_PROCESS_ID,
                TestData.SAWMILL_RELEASE_CONDITIONAL_VARS_PROCESS_ID);
    }
}
