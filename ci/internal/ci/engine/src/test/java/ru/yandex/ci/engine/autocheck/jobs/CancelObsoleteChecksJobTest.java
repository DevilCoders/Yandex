package ru.yandex.ci.engine.autocheck.jobs;

import java.util.List;
import java.util.Map;

import io.github.benas.randombeans.api.EnhancedRandom;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.CommonYdbTestBase;
import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementId;
import ru.yandex.ci.client.storage.StorageApiClient;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.launch.LaunchFlowInfo;
import ru.yandex.ci.core.launch.LaunchPullRequestInfo;
import ru.yandex.ci.core.launch.LaunchVcsInfo;
import ru.yandex.ci.core.pr.PullRequestVcsInfo;
import ru.yandex.ci.core.pr.RevisionNumberService;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.autocheck.AutocheckBlacklistService;
import ru.yandex.ci.engine.autocheck.AutocheckLaunchProcessor;
import ru.yandex.ci.engine.autocheck.AutocheckLaunchProcessorPostCommits;
import ru.yandex.ci.engine.autocheck.AutocheckService;
import ru.yandex.ci.engine.autocheck.jobs.autocheck.CancelObsoleteChecksJob;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.helpers.TestJobContext;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.storage.api.StorageApi.FindCheckByRevisionsResponse;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.test.random.TestRandomUtils;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static ru.yandex.ci.engine.autocheck.AutocheckBootstrapServicePullRequests.TRUNK_PRECOMMIT_PROCESS_ID;

@ContextConfiguration(classes = {
        CancelObsoleteChecksJob.class,
        CancelObsoleteChecksJobTest.Config.class,
})
@MockBean(AutocheckLaunchProcessor.class)
@MockBean(AutocheckLaunchProcessorPostCommits.class)
@MockBean(StorageApiClient.class)
@MockBean(AutocheckBlacklistService.class)
@MockBean(RevisionNumberService.class)
class CancelObsoleteChecksJobTest extends CommonYdbTestBase {

    private static final long SEED = -132185L;
    private static final EnhancedRandom RANDOM = TestRandomUtils.enhancedRandom(SEED);

    @MockBean
    StorageApiClient storageApiClient;

    @Autowired
    CancelObsoleteChecksJob cancelObsoleteChecksJob;

    @Test
    void execute_whenNoCheckFound() throws Exception {
        doReturn(FindCheckByRevisionsResponse.getDefaultInstance())
                .when(storageApiClient).findChecksByRevisionsAndTags(any());

        var context = createJobContext(TRUNK_PRECOMMIT_PROCESS_ID);
        cancelObsoleteChecksJob.execute(context);
        verify(storageApiClient, times(0)).cancelCheck(any());
    }

    @Test
    void execute_whenOneCheckFound() throws Exception {
        doReturn(FindCheckByRevisionsResponse.newBuilder()
                .addAllChecks(List.of(
                        protoCheck("check1")
                ))
                .build()
        ).when(storageApiClient).findChecksByRevisionsAndTags(any());

        var context = createJobContext(TRUNK_PRECOMMIT_PROCESS_ID);
        cancelObsoleteChecksJob.execute(context);
        verify(storageApiClient, times(1)).cancelCheck(eq("check1"));
    }

    @Test
    void execute_whenManyChecksFound() throws Exception {
        doReturn(FindCheckByRevisionsResponse.newBuilder()
                .addAllChecks(List.of(
                        protoCheck("check1"),
                        protoCheck("check2")
                ))
                .build()
        ).when(storageApiClient).findChecksByRevisionsAndTags(any());

        var context = createJobContext(TRUNK_PRECOMMIT_PROCESS_ID);
        cancelObsoleteChecksJob.execute(context);
        verify(storageApiClient, times(1)).cancelCheck(eq("check1"));
        verify(storageApiClient, times(1)).cancelCheck(eq("check2"));
    }

    private static CheckOuterClass.Check protoCheck(String id) {
        return CheckOuterClass.Check.newBuilder()
                .setId(id)
                .setDiffSetId(100L)
                .setOwner("owner")
                .build();
    }

    private static TestJobContext createJobContext(CiProcessId processId) {
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
                null
        );

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
                                .previousRevision(TestData.TRUNK_R2)
                                .revision(TestData.DIFF_SET_1.getOrderedMergeRevision())
                                .pullRequestInfo(pullRequestInfo)
                                .build()
                )
                .jobs(Map.of())
                .build();

        return new TestJobContext(flowLaunchEntity);
    }

    @Configuration
    static class Config {
        @Bean
        AutocheckService autocheckService(
                AutocheckLaunchProcessor autocheckLaunchProcessor,
                AutocheckLaunchProcessorPostCommits autocheckLaunchProcessorPostCommits,
                StorageApiClient storageApiClient,
                CiMainDb db,
                AutocheckBlacklistService autocheckBlacklistService,
                RevisionNumberService revisionNumberService

        ) {
            return new AutocheckService(autocheckLaunchProcessor, autocheckLaunchProcessorPostCommits,
                    storageApiClient, db, autocheckBlacklistService, revisionNumberService);
        }
    }

}
