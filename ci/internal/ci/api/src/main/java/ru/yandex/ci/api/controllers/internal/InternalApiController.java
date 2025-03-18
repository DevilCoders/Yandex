package ru.yandex.ci.api.controllers.internal;

import com.google.common.collect.Iterables;
import com.google.protobuf.Empty;
import io.grpc.stub.StreamObserver;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.api.internal.InternalApiGrpc;
import ru.yandex.ci.api.internal.InternalApiOuterClass.Time;
import ru.yandex.ci.common.grpc.GrpcUtils;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.engine.event.LaunchMappers;
import ru.yandex.ci.engine.timeline.CommitFetchService;
import ru.yandex.ci.engine.timeline.CommitFetchService.CommitOffset;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.FullJobLaunchId;
import ru.yandex.ci.flow.engine.runtime.state.JobProgressService;
import ru.yandex.ci.flow.engine.runtime.state.model.TaskBadge;
import ru.yandex.ci.flow.utils.UrlService;
import ru.yandex.ci.job.GetCommitsRequest;
import ru.yandex.ci.job.GetCommitsResponse;
import ru.yandex.ci.job.JobInstanceId;
import ru.yandex.ci.job.TaskletProgress;
import ru.yandex.ci.util.HostnameUtils;

@Slf4j
@RequiredArgsConstructor
public class InternalApiController extends InternalApiGrpc.InternalApiImplBase {
    private final JobProgressService jobProgressService;
    private final CommitFetchService commitFetchService;
    private final UrlService urlService;
    private final CiDb db;

    @Override
    public void ping(Empty request, StreamObserver<Time> responseObserver) {
        responseObserver.onNext(Time.newBuilder()
                .setHost(HostnameUtils.getHostname())
                .setTimestampMillis(System.currentTimeMillis())
                .build());
        responseObserver.onCompleted();
    }


    @Override
    public void updateTaskletProgress(TaskletProgress request, StreamObserver<Empty> responseObserver) {
        try {
            JobInstanceId jobInstanceId = request.getJobInstanceId();
            log.info("Update progress from {} to {}", jobInstanceId, request.getProgress());

            FullJobLaunchId jobLaunchId = new FullJobLaunchId(
                    FlowLaunchId.of(jobInstanceId.getFlowLaunchId()),
                    jobInstanceId.getJobId(),
                    jobInstanceId.getNumber()
            );

            String id = request.getId();
            if (id.isEmpty()) {
                id = TaskBadge.DEFAULT_ID;
            }
            // запрещаем обновлять статусы, которые проставляет сам ci
            TaskBadge.checkNotReservedId(id);

            jobProgressService.changeTaskBadge(
                    jobLaunchId,
                    TaskBadge.of(
                            id,
                            request.getModule(),
                            request.getUrl(),
                            TaskBadge.TaskStatus.valueOf(request.getStatus().name()),
                            request.getProgress(),
                            request.getText(),
                            false
                    )
            );

            responseObserver.onNext(Empty.getDefaultInstance());
            responseObserver.onCompleted();
        } catch (Exception e) {
            throw GrpcUtils.internalError(e);
        }
    }

    @Override
    public void getCommits(GetCommitsRequest request, StreamObserver<GetCommitsResponse> responseObserver) {
        var includeFromActiveReleases = switch (request.getType()) {
            case FROM_PREVIOUS_ACTIVE -> false;
            case FROM_PREVIOUS_STABLE, UNRECOGNIZED -> true;
        };
        var offset = request.hasOffset()
                ? CommitOffset.of(ArcBranch.ofString(request.getOffset().getBranch()), request.getOffset().getNumber())
                : CommitOffset.empty();
        var limit = request.getLimit();
        if (limit <= 0) {
            limit = 100;
            log.info("limit not provided or negative ({}), default used ({})", request.getLimit(), limit);
        }

        var limitFinal = limit;
        var result = db.currentOrReadOnly(() -> {
            var flowLaunchId = FlowLaunchId.of(request.getFlowLaunchId());
            var flowLaunch = db.flowLaunch().findOptional(flowLaunchId)
                    .orElseThrow(() -> GrpcUtils.notFoundException("FlowLaunch not found " + flowLaunchId));
            return commitFetchService.fetchTimelineCommits(flowLaunch.getLaunchId(),
                    offset, limitFinal, includeFromActiveReleases
            );
        });
        var builder = GetCommitsResponse.newBuilder();

        result.items()
                .forEach(timelineCommit -> {
                    var commit = LaunchMappers.commit(timelineCommit.getCommit());
                    var timelineCommitBuilder = ru.yandex.ci.job.TimelineCommit.newBuilder()
                            .setCommit(commit);

                    if (timelineCommit.getRelease() != null) {
                        timelineCommitBuilder.setRelease(
                                LaunchMappers.release(
                                        timelineCommit.getRelease(), urlService.toLaunch(timelineCommit.getRelease())
                                )
                        );
                    }

                    builder.addTimelineCommits(timelineCommitBuilder.build());
                    builder.addCommits(commit);
                });

        if (result.hasMore()) {
            builder.setNext(LaunchMappers.commitOffset(Iterables.getLast(result.items()).getCommit()));
        }

        responseObserver.onNext(builder.build());
        responseObserver.onCompleted();
    }

}
