package ru.yandex.ci.common.temporal.logging;

import java.time.Instant;
import java.time.ZoneId;
import java.time.temporal.ChronoUnit;
import java.util.List;
import java.util.function.Consumer;

import com.google.protobuf.Timestamp;
import io.grpc.inprocess.InProcessServerBuilder;
import io.grpc.stub.StreamObserver;
import io.temporal.api.workflow.v1.WorkflowExecutionInfo;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.RegisterExtension;
import org.mockito.Mockito;
import vtail.api.core.Log;
import vtail.api.query.Api;
import vtail.api.query.QueryGrpc;

import ru.yandex.ci.client.logs.LogsClient;
import ru.yandex.ci.common.grpc.GrpcCleanupExtension;
import ru.yandex.ci.common.grpc.GrpcClientPropertiesStub;
import ru.yandex.ci.common.grpc.GrpcTestUtils;
import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.common.temporal.TemporalService;

import static org.assertj.core.api.Assertions.assertThat;
import static ru.yandex.ci.common.temporal.logging.TemporalLogService.TIME_SHIFT;

class TemporalLogServiceTest {

    @RegisterExtension
    private final GrpcCleanupExtension grpcCleanup = new GrpcCleanupExtension();

    private static final Instant START_TIME = Instant.ofEpochMilli(1652865730321L); //2022-05-18 12:22:10.321
    private static final Instant END_TIME = START_TIME.plus(1, ChronoUnit.HOURS);

    @Test
    void getLog() {
        String workflowId = "WF-42";

        Consumer<Api.FetchLogsRequest> requestValidator = request -> {
            assertThat(ProtoConverter.convert(request.getStart())).isEqualTo(START_TIME.minus(TIME_SHIFT));
            assertThat(ProtoConverter.convert(request.getEnd())).isEqualTo(END_TIME.plus(TIME_SHIFT));
            assertThat(request.getQuery()).isEqualTo(
                    "env=local project=ci service=ci-tms task_id=" + workflowId
            );
            assertThat(request.getLimit()).isEqualTo(10000);
            assertThat(request.getSortDir()).isEqualTo(Api.Sort.DESC);
        };

        String channelName = InProcessServerBuilder.generateName();
        var server = GrpcTestUtils.createAndStartServer(channelName, List.of(new TestLogsService(requestValidator)));
        grpcCleanup.register(server);

        var properties = GrpcClientPropertiesStub.of(channelName, grpcCleanup);

        var logClient = LogsClient.create(properties);
        var temporalService = Mockito.mock(TemporalService.class);
        var executionInfo = WorkflowExecutionInfo.newBuilder()
                .setStartTime(ProtoConverter.convert(START_TIME))
                .setCloseTime(ProtoConverter.convert(END_TIME))
                .build();

        Mockito.when(temporalService.getWorkflowExecutionInfo(workflowId))
                .thenReturn(executionInfo);

        TemporalLogService logService = new TemporalLogService(
                temporalService,
                logClient,
                "ci",
                "ci-tms",
                "local",
                ZoneId.of("Europe/Moscow")
        );

        assertThat(logService.getLog(workflowId))
                .isEqualTo("""

                        ===========================================================
                        2022-05-18 12:22:11.234
                        Activity ID: activity42
                        Run ID: run21
                        Host: host1
                        ===========================================================

                        2022-05-18 12:22:11.234  [TemporalLogServiceTest] line1
                        2022-05-18 12:22:11.234  [TemporalLogServiceTest] line2
                        2022-05-18 12:22:11.234  [TemporalLogServiceTest] line3

                        ===========================================================
                        2022-05-18 12:22:11.234
                        Activity ID: activity42
                        Run ID: run21
                        Host: host2
                        ===========================================================

                        2022-05-18 12:22:11.234  [TemporalLogServiceTest] next line
                        """
                );

    }

    private static class TestLogsService extends QueryGrpc.QueryImplBase {

        private final Consumer<Api.FetchLogsRequest> requestValidator;

        private TestLogsService(Consumer<Api.FetchLogsRequest> requestValidator) {
            this.requestValidator = requestValidator;
        }

        @Override
        public void fetchLogs(Api.FetchLogsRequest request, StreamObserver<Api.FetchLogsRecord> responseObserver) {

            requestValidator.accept(request);

            String customFields = """
                    {
                        "run_id": "run21",
                        "activity_id": "activity42"
                    }
                    """;

            var message = Log.LogMessage.newBuilder()
                    .setTime(Timestamp.newBuilder().setSeconds(1652865730).setNanos(1234567890).build())
                    .setComponent("ru.yandex.ci.common.temporal.logging.TemporalLogServiceTest")
                    .setHostname("host2")
                    .setRest(customFields);

            responseObserver.onNext(
                    Api.FetchLogsRecord.newBuilder()
                            .setMsg(message.setMessage("next line"))
                            .build());

            message.setHostname("host1");

            for (int i = 3; i > 0; i--) {
                responseObserver.onNext(
                        Api.FetchLogsRecord.newBuilder()
                                .setMsg(message.setMessage("line" + i))
                                .build());

            }

            responseObserver.onCompleted();

        }
    }

}
