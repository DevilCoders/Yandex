package ru.yandex.ci.engine.autocheck;

import java.util.List;
import java.util.Map;

import io.github.benas.randombeans.api.EnhancedRandom;
import org.assertj.core.api.Assertions;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementId;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.core.launch.LaunchFlowInfo;
import ru.yandex.ci.core.launch.LaunchPullRequestInfo;
import ru.yandex.ci.core.launch.LaunchVcsInfo;
import ru.yandex.ci.core.pr.PullRequestVcsInfo;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.autocheck.jobs.autocheck.StartAutocheckJob;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.helpers.TestJobContext;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.test.random.TestRandomUtils;

import static ru.yandex.ci.engine.autocheck.AutocheckBootstrapServicePullRequests.BRANCH_PRECOMMIT_PROCESS_ID;
import static ru.yandex.ci.engine.autocheck.AutocheckBootstrapServicePullRequests.TRUNK_PRECOMMIT_PROCESS_ID;

class StartAutocheckJobTest {

    private static final long SEED = -836825185L;
    private static final EnhancedRandom RANDOM = TestRandomUtils.enhancedRandom(SEED);

    @Test
    void gsidBasePrecommit() {
        Assertions.assertThat(generateGsidBase(BRANCH_PRECOMMIT_PROCESS_ID))
                .isEqualTo(
                        "ARCANUM:4221 " +
                                "ARCANUM_DIFF_SET:777 " +
                                "ARC_MERGE:ds2 " +
                                "CI_CHECK_OWNER:andreevdm " +
                                "CI:autocheck/a.yaml:ACTION:autocheck-branch-precommits:42 " +
                                "CI_FLOW:flow-id"
                );

        Assertions.assertThat(generateGsidBase(TRUNK_PRECOMMIT_PROCESS_ID))
                .isEqualTo(
                        "ARCANUM:4221 " +
                                "ARCANUM_DIFF_SET:777 " +
                                "ARC_MERGE:ds2 " +
                                "CI_CHECK_OWNER:andreevdm " +
                                "CI:autocheck/a.yaml:ACTION:autocheck-trunk-precommits:42 " +
                                "CI_FLOW:flow-id"
                );
    }

    private String generateGsidBase(CiProcessId processId) {
        var pullRequestVcsInfo = new PullRequestVcsInfo(
                TestData.DS2_REVISION,
                TestData.TRUNK_R2.toRevision(),
                ArcBranch.trunk(),
                TestData.REVISION,
                TestData.USER_BRANCH
        );

        var pullRequestInfo = new LaunchPullRequestInfo(
                4221,
                777,
                TestData.CI_USER,
                "Some summary",
                "Some description",
                ArcanumMergeRequirementId.of("x", "y"),
                pullRequestVcsInfo,
                List.of("CI-1"),
                List.of("label1"),
                null);

        var flowLaunchEntity = RANDOM.nextObject(FlowLaunchEntity.class).toBuilder()
                .processId(processId.asString())
                .launchNumber(42)
                .id(FlowLaunchId.of("flow-id"))
                .flowInfo(
                        RANDOM.nextObject(LaunchFlowInfo.class).toBuilder()
                                .flowId(FlowFullId.of(processId.getPath(), "autocheck"))
                                .build()
                )
                .vcsInfo(
                        LaunchVcsInfo.builder()
                                .revision(TestData.DIFF_SET_2.getOrderedMergeRevision())
                                .commit(TestData.DS2_COMMIT)
                                .pullRequestInfo(pullRequestInfo)
                                .build()
                )
                .jobs(Map.of())
                .build();

        var context = new TestJobContext(flowLaunchEntity);
        return StartAutocheckJob.generateGsidBase(context);
    }

}
