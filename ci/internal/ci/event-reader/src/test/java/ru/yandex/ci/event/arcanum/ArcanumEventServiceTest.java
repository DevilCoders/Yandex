package ru.yandex.ci.event.arcanum;

import lombok.extern.slf4j.Slf4j;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.Timeout;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.mock.mockito.SpyBean;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.arcanum.event.ArcanumEvents;
import ru.yandex.ci.common.bazinga.TestBazingaUtils;
import ru.yandex.ci.common.grpc.ProtobufTestUtils;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.autocheck.AutocheckConstants;
import ru.yandex.ci.core.pr.PullRequestDiffSet;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.discovery.task.PullRequestDiffSetProcessedByCiTask;
import ru.yandex.ci.engine.flow.YavDelegationException;
import ru.yandex.ci.engine.pr.PullRequestService;
import ru.yandex.ci.engine.spring.clients.ArcanumClientConfig;
import ru.yandex.ci.event.spring.ArcanumServiceConfig;
import ru.yandex.ci.event.spring.CiEventReaderPropertiesConfig;
import ru.yandex.ci.tms.test.AbstractEntireTest;
import ru.yandex.commune.bazinga.BazingaTaskManager;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.atLeastOnce;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

@Timeout(30)
@Slf4j
@ContextConfiguration(classes = {
        ArcanumClientConfig.class,
        ArcanumServiceConfig.class,
        CiEventReaderPropertiesConfig.class
})
class ArcanumEventServiceTest extends AbstractEntireTest {

    @SuppressWarnings("HidingField") // Rewrite `spy` to something better
    @SpyBean
    protected BazingaTaskManager bazingaTaskManager;

    @SpyBean
    private PullRequestService pullRequestService;

    @Autowired
    private ArcanumEventService service;

    private PullRequestDiffSet.Id id1;
    private PullRequestDiffSet.Id id2;

    @BeforeEach
    void beforeEach() throws YavDelegationException {
        id1 = PullRequestDiffSet.Id.of(1785172, 3736581);
        id2 = PullRequestDiffSet.Id.of(1785172, 3736597);

        discoveryServicePostCommits.processPostCommit(ArcBranch.trunk(), TestData.TRUNK_R2.toRevision(), false);
        engineTester.delegateToken(AutocheckConstants.AUTOCHECK_A_YAML_PATH);
    }

    @Test
    void updatedOnly() throws InterruptedException {
        mockArcanumClientResponse(id1);
        service.processEvent(readProtobuf("arcanum/diff-set-is-ready-for-auto-check-1.pb"), null);
        waitForStatus(id1, PullRequestDiffSet.Status.DISCOVERED);

        verify(bazingaTaskManager, never()).schedule(any(PullRequestDiffSetProcessedByCiTask.class));
    }

    @Test
    void updatedThenUpdatedOnly() throws InterruptedException {
        mockArcanumClientResponse(id1);
        service.processEvent(readProtobuf("arcanum/diff-set-is-ready-for-auto-check-1.pb"), null);
        waitForStatus(id1, PullRequestDiffSet.Status.DISCOVERED);

        mockArcanumClientResponse(id2);
        service.processEvent(readProtobuf("arcanum/diff-set-is-ready-for-auto-check-2.pb"), null);
        waitForStatus(id2, PullRequestDiffSet.Status.DISCOVERED);
        waitForStatus(id1, PullRequestDiffSet.Status.COMPLETE); // complete

        verify(bazingaTaskManager, never()).schedule(any(PullRequestDiffSetProcessedByCiTask.class));
    }

    @Test
    void updatedThenUpdatedOnlyImmediate() throws InterruptedException {
        mockArcanumClientResponse(id1);
        service.processEvent(readProtobuf("arcanum/diff-set-is-ready-for-auto-check-1.pb"), null);
        mockArcanumClientResponse(id2);
        service.processEvent(readProtobuf("arcanum/diff-set-is-ready-for-auto-check-2.pb"), null);
        waitForStatus(id2, PullRequestDiffSet.Status.DISCOVERED);
        waitForStatus(id1, PullRequestDiffSet.Status.COMPLETE); // complete

        verify(bazingaTaskManager, never()).schedule(any(PullRequestDiffSetProcessedByCiTask.class));
    }

    @Test
    void updatedThenClosed() throws InterruptedException {
        mockArcanumClientResponse(id2);
        service.processEvent(readProtobuf("arcanum/diff-set-is-ready-for-auto-check-2.pb"), null);
        waitForStatus(id2, PullRequestDiffSet.Status.DISCOVERED);

        service.processEvent(readProtobuf("arcanum/review-request-closed.pb"), null);
        waitForStatus(id2, PullRequestDiffSet.Status.COMPLETE);

        verify(bazingaTaskManager, never()).schedule(any(PullRequestDiffSetProcessedByCiTask.class));
    }

    @Test
    void updatedThenClosedImmediately() throws InterruptedException {
        mockArcanumClientResponse(id2);
        service.processEvent(readProtobuf("arcanum/diff-set-is-ready-for-auto-check-2.pb"), null);
        service.processEvent(readProtobuf("arcanum/review-request-closed.pb"), null);
        waitForStatus(id2, PullRequestDiffSet.Status.COMPLETE);

        verify(bazingaTaskManager, never()).schedule(any(PullRequestDiffSetProcessedByCiTask.class));
    }

    @Test
    void updatedThenUpdatedThenClosed() throws InterruptedException {
        mockArcanumClientResponse(id1);
        service.processEvent(readProtobuf("arcanum/diff-set-is-ready-for-auto-check-1.pb"), null);
        waitForStatus(id1, PullRequestDiffSet.Status.DISCOVERED);

        mockArcanumClientResponse(id2);
        service.processEvent(readProtobuf("arcanum/diff-set-is-ready-for-auto-check-2.pb"), null);
        waitForStatus(id2, PullRequestDiffSet.Status.DISCOVERED);
        waitForStatus(id1, PullRequestDiffSet.Status.COMPLETE);

        service.processEvent(readProtobuf("arcanum/review-request-closed.pb"), null);
        waitForStatus(id2, PullRequestDiffSet.Status.COMPLETE);

        verify(bazingaTaskManager, never()).schedule(any(PullRequestDiffSetProcessedByCiTask.class));
    }

    @Test
    void updatedThenUpdatedThenClosedImmediately() throws InterruptedException {
        mockArcanumClientResponse(id1);
        service.processEvent(readProtobuf("arcanum/diff-set-is-ready-for-auto-check-1.pb"), null);
        mockArcanumClientResponse(id2);
        service.processEvent(readProtobuf("arcanum/diff-set-is-ready-for-auto-check-2.pb"), null);
        service.processEvent(readProtobuf("arcanum/review-request-closed.pb"), null);
        waitForStatus(id1, PullRequestDiffSet.Status.COMPLETE);
        waitForStatus(id2, PullRequestDiffSet.Status.COMPLETE);

        verify(bazingaTaskManager, never()).schedule(any(PullRequestDiffSetProcessedByCiTask.class));
    }

    @Test
    void discardedOnly() throws InterruptedException {
        service.processEvent(readProtobuf("arcanum/review-request-closed.pb"), null);
        waitForStatus(id2, PullRequestDiffSet.Status.SKIP);

        verify(bazingaTaskManager, never()).schedule(any(PullRequestDiffSetProcessedByCiTask.class));
    }

    @Test
    void updateOnlyFromIgnoredBranch() throws InterruptedException {
        mockArcanumClientResponse(id1);
        var scheduledJobs = TestBazingaUtils.spyScheduledJobs(bazingaTaskManager);

        service.processEvent(readProtobuf("arcanum/diff-set-is-ready-for-auto-check__to-ignored-branch__1.pb"), null);

        verify(bazingaTaskManager, atLeastOnce()).schedule(any(PullRequestDiffSetProcessedByCiTask.class));
        var bazingaJobId = TestBazingaUtils.getFirst(scheduledJobs, PullRequestDiffSetProcessedByCiTask.class);
        TestBazingaUtils.waitOrFail(bazingaTaskManager, bazingaJobId);
        verify(pullRequestService, atLeastOnce())
                .skipArcanumDefaultChecks(eq(id1.getPullRequestId()), eq(id1.getDiffSetId()));
    }


    private void waitForStatus(PullRequestDiffSet.Id id, PullRequestDiffSet.Status expectStatus)
            throws InterruptedException {
        while (true) {
            var diffSetOpt = db.currentOrReadOnly(() -> db.pullRequestDiffSetTable().find(id));
            if (diffSetOpt.isPresent()) {
                var diffSet = diffSetOpt.get();
                log.info("Diff set {} status = {}, expected = {}", id, diffSet.getStatus(), expectStatus);
                if (diffSet.getStatus() == expectStatus) {
                    return; // ---
                }
            } else {
                log.info("Diff set {} is empty, expected = {}", id, expectStatus);
            }
            //noinspection BusyWait
            Thread.sleep(500);
        }
    }

    private static ArcanumEvents.Event readProtobuf(String source) {
        return ProtobufTestUtils.parseProtoText(source, ArcanumEvents.Event.class);
    }

    private void mockArcanumClientResponse(PullRequestDiffSet.Id id1) {
        arcanumTestServer.mockGetReviewRequestData(
                id1.getPullRequestId(),
                "arcanum_responses/getReviewRequestData-pr%d-ds%d.json".formatted(
                        id1.getPullRequestId(),
                        id1.getDiffSetId()
                )
        );
    }

}
