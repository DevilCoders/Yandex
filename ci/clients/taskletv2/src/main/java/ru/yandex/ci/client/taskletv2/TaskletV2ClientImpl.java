package ru.yandex.ci.client.taskletv2;

import java.util.function.Supplier;

import io.grpc.StatusRuntimeException;

import ru.yandex.ci.client.tvm.grpc.OAuthCallCredentials;
import ru.yandex.ci.common.grpc.GrpcClient;
import ru.yandex.ci.common.grpc.GrpcClientImpl;
import ru.yandex.ci.common.grpc.GrpcClientProperties;
import ru.yandex.ci.common.grpc.GrpcUtils;
import ru.yandex.ci.common.grpc.UnrecoverableExceptionSource;
import ru.yandex.tasklet.api.v2.SchemaRegistryServiceGrpc;
import ru.yandex.tasklet.api.v2.SchemaRegistryServiceGrpc.SchemaRegistryServiceBlockingStub;
import ru.yandex.tasklet.api.v2.SchemaRegistryServiceOuterClass.GetSchemaRequest;
import ru.yandex.tasklet.api.v2.SchemaRegistryServiceOuterClass.GetSchemaResponse;
import ru.yandex.tasklet.api.v2.TaskletServiceGrpc;
import ru.yandex.tasklet.api.v2.TaskletServiceGrpc.TaskletServiceBlockingStub;
import ru.yandex.tasklet.api.v2.TaskletServiceOuterClass.ExecuteRequest;
import ru.yandex.tasklet.api.v2.TaskletServiceOuterClass.ExecuteResponse;
import ru.yandex.tasklet.api.v2.TaskletServiceOuterClass.GetBuildRequest;
import ru.yandex.tasklet.api.v2.TaskletServiceOuterClass.GetBuildResponse;
import ru.yandex.tasklet.api.v2.TaskletServiceOuterClass.GetExecutionRequest;
import ru.yandex.tasklet.api.v2.TaskletServiceOuterClass.GetExecutionResponse;
import ru.yandex.tasklet.api.v2.TaskletServiceOuterClass.GetLabelRequest;
import ru.yandex.tasklet.api.v2.TaskletServiceOuterClass.GetLabelResponse;

public class TaskletV2ClientImpl implements TaskletV2Client {

    private final GrpcClient grpcClient;
    private final Supplier<TaskletServiceBlockingStub> taskletStub;
    private final Supplier<SchemaRegistryServiceBlockingStub> schemaStub;
    private final UnrecoverableExceptionSource exceptionSource;

    private TaskletV2ClientImpl(GrpcClientProperties properties, UnrecoverableExceptionSource exceptionSource) {
        this.grpcClient = GrpcClientImpl.builder(properties, getClass())
                .excludeLoggingFullResponse(SchemaRegistryServiceGrpc.getGetSchemaMethod())
                .build();
        this.taskletStub = grpcClient.buildStub(TaskletServiceGrpc::newBlockingStub);
        this.schemaStub = grpcClient.buildStub(SchemaRegistryServiceGrpc::newBlockingStub);
        this.exceptionSource = exceptionSource;
    }

    public static TaskletV2ClientImpl create(
            GrpcClientProperties properties,
            UnrecoverableExceptionSource exceptionSource
    ) {
        return new TaskletV2ClientImpl(properties, exceptionSource);
    }

    @Override
    public void close() throws Exception {
        grpcClient.close();
    }

    @Override
    public GetLabelResponse getLabel(GetLabelRequest request) {
        try {
            return taskletStub.get().getLabel(request);
        } catch (StatusRuntimeException e) {
            throw GrpcUtils.wrapRepeatableException(e, request, "Unable to get Tasklet V2 label", exceptionSource);
        }
    }

    @Override
    public GetBuildResponse getBuild(GetBuildRequest request) {
        try {
            return taskletStub.get().getBuild(request);
        } catch (StatusRuntimeException e) {
            throw GrpcUtils.wrapRepeatableException(e, request, "Unable to get Tasklet V2 build", exceptionSource);
        }
    }

    @Override
    public GetSchemaResponse getSchema(GetSchemaRequest request) {
        try {
            return schemaStub.get().getSchema(request);
        } catch (StatusRuntimeException e) {
            throw GrpcUtils.wrapRepeatableException(e, request, "Unable to get Tasklet V2 schema", exceptionSource);
        }

    }

    // Actual tasklet execution with provided account

    @Override
    public AuthenticatedExecutor getExecutor(String oauthToken) {
        var credentials = new OAuthCallCredentials(oauthToken, true);

        return new AuthenticatedExecutor() {
            @Override
            public ExecuteResponse execute(ExecuteRequest request) {
                try {
                    return stub().execute(request);
                } catch (StatusRuntimeException e) {
                    throw GrpcUtils.wrapRepeatableException(e, request,
                            "Unable to get Tasklet V2 execution", exceptionSource);
                }
            }

            @Override
            public GetExecutionResponse getExecution(GetExecutionRequest request) {
                try {
                    return stub().getExecution(request);
                } catch (StatusRuntimeException e) {
                    throw GrpcUtils.wrapRepeatableException(e, request,
                            "Unable to get Tasklet V2 execution", exceptionSource);
                }
            }

            private TaskletServiceBlockingStub stub() {
                return taskletStub.get().withCallCredentials(credentials);
            }
        };
    }
}
