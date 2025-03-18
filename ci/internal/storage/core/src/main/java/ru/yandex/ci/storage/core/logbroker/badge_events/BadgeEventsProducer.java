package ru.yandex.ci.storage.core.logbroker.badge_events;

import java.util.Objects;
import java.util.Optional;

import com.google.common.base.Preconditions;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.core.CiBadgeEvents;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.commune.bazinga.BazingaTaskManager;

@Slf4j
public class BadgeEventsProducer {

    private final BazingaTaskManager bazingaTaskManager;

    public BadgeEventsProducer(BazingaTaskManager bazingaTaskManager) {
        this.bazingaTaskManager = bazingaTaskManager;
    }

    public void onCheckCreated(CheckEntity check) {
        Preconditions.checkArgument(check.getStatus() == Common.CheckStatus.CREATED,
                "check %s status is %s, %s expected", check.getId(), check.getStatus(), Common.CheckStatus.CREATED
        );

        var builder = CiBadgeEvents.CheckCreatedEvent.newBuilder()
                .setCheck(makeCheck(check))
                .setAuthor(makeAuthor(check));

        makeReviewRequest(check).ifPresent(builder::setReviewRequest);

        var event = CiBadgeEvents.Event.newBuilder()
                .setCheckCreatedEvent(builder.build())
                .build();

        produceEvent(check, event);
    }

    public void onCheckFinished(CheckEntity check) {
        var status = makeCheckStatus(check.getStatus());
        Preconditions.checkState(status != Common.CheckStatus.RUNNING,
                "unexpected status " + check.getStatus() + " for check " + check.getId());

        var builder = CiBadgeEvents.CheckFinishedEvent.newBuilder()
                .setCheck(makeCheck(check))
                .setAuthor(makeAuthor(check))
                .setStatus(status);

        makeReviewRequest(check).ifPresent(builder::setReviewRequest);

        var event = CiBadgeEvents.Event.newBuilder()
                .setCheckFinishedEvent(builder.build())
                .build();

        produceEvent(check, event);
    }

    public void onCheckFirstFail(CheckEntity check,
                                 CheckIterationEntity iteration) {

        var builder = CiBadgeEvents.CheckFirstFailEvent.newBuilder()
                .setCheck(makeCheck(check))
                .setAuthor(makeAuthor(check))
                .setIteration(makeIteration(iteration));

        makeReviewRequest(check).ifPresent(builder::setReviewRequest);

        var event = CiBadgeEvents.Event.newBuilder()
                .setFirstFailTestsEvent(builder)
                .build();

        produceEvent(check, event);
    }

    private static Common.CheckStatus makeCheckStatus(Common.CheckStatus status) {
        return switch (status) {
            case COMPLETED_SUCCESS -> Common.CheckStatus.COMPLETED_SUCCESS;
            case CANCELLED, CANCELLED_BY_TIMEOUT -> Common.CheckStatus.CANCELLED;
            case COMPLETED_FAILED, COMPLETED_WITH_FATAL_ERROR -> Common.CheckStatus.COMPLETED_FAILED;
            case CANCELLING, CANCELLING_BY_TIMEOUT, COMPLETING,
                    CREATED, RUNNING, UNRECOGNIZED -> Common.CheckStatus.RUNNING;
        };
    }

    public void onIterationTypeFinished(CheckEntity check, CheckIterationEntity iteration) {
        var dovecoteIteration = makeIteration(iteration);
        var builder = CiBadgeEvents.IterationFinishedEvent.newBuilder()
                .setCheck(makeCheck(check))
                .setAuthor(makeAuthor(check))
                .setIteration(dovecoteIteration)
                .setStatus(dovecoteIteration.getStatus());

        makeReviewRequest(check).ifPresent(builder::setReviewRequest);

        var event = CiBadgeEvents.Event.newBuilder()
                .setIterationFinishedEvent(builder.build())
                .build();

        produceEvent(check, event);
    }

    private CiBadgeEvents.DovecoteIteration makeIteration(CheckIterationEntity iteration) {
        var status = switch (iteration.getStatus()) {
            case COMPLETED_SUCCESS -> Common.CheckStatus.COMPLETED_SUCCESS;
            case CANCELLED, CANCELLED_BY_TIMEOUT -> Common.CheckStatus.CANCELLED;
            case COMPLETED_FAILED, COMPLETED_WITH_FATAL_ERROR -> Common.CheckStatus.COMPLETED_FAILED;
            case CANCELLING, CANCELLING_BY_TIMEOUT, COMPLETING,
                    CREATED, RUNNING, UNRECOGNIZED -> Common.CheckStatus.RUNNING;
        };

        return CiBadgeEvents.DovecoteIteration.newBuilder()
                .setId(CheckProtoMappers.toProtoIterationId(iteration.getId()))
                .setStatus(status)
                .build();
    }

    private static CiBadgeEvents.DovecoteCheck makeCheck(CheckEntity check) {
        return CiBadgeEvents.DovecoteCheck.newBuilder()
                .setId(check.getId().getId())
                .setStatus(makeCheckStatus(check.getStatus()))
                .build();
    }

    private static CiBadgeEvents.User makeAuthor(CheckEntity check) {
        return CiBadgeEvents.User.newBuilder().setName(check.getAuthor()).build();
    }

    private static Optional<CiBadgeEvents.ReviewRequest> makeReviewRequest(CheckEntity check) {
        return check.getPullRequestId()
                .map(pullRequestId -> {
                            var diffSetId = Objects.requireNonNullElse(check.getDiffSetId(), 0L);
                            return CiBadgeEvents.ReviewRequest.newBuilder()
                                    .setId(pullRequestId)
                                    .setDiffsetId(diffSetId)
                                    .build();
                        }
                );
    }

    private void produceEvent(CheckEntity check, CiBadgeEvents.Event event) {
        if (check.isNotificationsDisabled()) {
            log.info("skip event {}, cause notifications disabled in check {}", event.getTypeCase(), check.getId());
            return;
        }
        bazingaTaskManager.schedule(new BadgeEventSendTask(check.getId(), event));
    }
}
