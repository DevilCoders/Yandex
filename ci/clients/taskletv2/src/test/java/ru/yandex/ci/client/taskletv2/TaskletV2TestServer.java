package ru.yandex.ci.client.taskletv2;

import java.time.Clock;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.UUID;
import java.util.concurrent.ExecutorService;
import java.util.function.Function;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import com.google.protobuf.DescriptorProtos.FileDescriptorSet;
import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.Message;
import com.google.protobuf.TextFormat;
import io.grpc.Server;
import io.grpc.stub.StreamObserver;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.common.grpc.GrpcTestUtils;
import ru.yandex.ci.common.grpc.GrpcUtils;
import ru.yandex.ci.common.grpc.ProtobufTestUtils;
import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.common.proto.ProtoUtils;
import ru.yandex.ci.util.Clearable;
import ru.yandex.ci.util.MapsContainer;
import ru.yandex.tasklet.TaskletAction;
import ru.yandex.tasklet.api.v2.DataModel;
import ru.yandex.tasklet.api.v2.DataModel.Build;
import ru.yandex.tasklet.api.v2.DataModel.BuildMeta;
import ru.yandex.tasklet.api.v2.DataModel.BuildSpec;
import ru.yandex.tasklet.api.v2.DataModel.EExecutionStatus;
import ru.yandex.tasklet.api.v2.DataModel.Execution;
import ru.yandex.tasklet.api.v2.DataModel.IOSchema;
import ru.yandex.tasklet.api.v2.DataModel.IOSimpleSchemaProto;
import ru.yandex.tasklet.api.v2.DataModel.Label;
import ru.yandex.tasklet.api.v2.DataModel.LabelMeta;
import ru.yandex.tasklet.api.v2.DataModel.LabelSpec;
import ru.yandex.tasklet.api.v2.DataModel.ProcessingResult;
import ru.yandex.tasklet.api.v2.SchemaRegistryServiceGrpc.SchemaRegistryServiceImplBase;
import ru.yandex.tasklet.api.v2.SchemaRegistryServiceOuterClass.GetSchemaRequest;
import ru.yandex.tasklet.api.v2.SchemaRegistryServiceOuterClass.GetSchemaResponse;
import ru.yandex.tasklet.api.v2.TaskletServiceGrpc.TaskletServiceImplBase;
import ru.yandex.tasklet.api.v2.TaskletServiceOuterClass.ExecuteRequest;
import ru.yandex.tasklet.api.v2.TaskletServiceOuterClass.ExecuteResponse;
import ru.yandex.tasklet.api.v2.TaskletServiceOuterClass.GetBuildRequest;
import ru.yandex.tasklet.api.v2.TaskletServiceOuterClass.GetBuildResponse;
import ru.yandex.tasklet.api.v2.TaskletServiceOuterClass.GetExecutionRequest;
import ru.yandex.tasklet.api.v2.TaskletServiceOuterClass.GetExecutionResponse;
import ru.yandex.tasklet.api.v2.TaskletServiceOuterClass.GetLabelRequest;
import ru.yandex.tasklet.api.v2.TaskletServiceOuterClass.GetLabelResponse;
import ru.yandex.tasklet.test.TaskletContextStub;

@Slf4j
public class TaskletV2TestServer implements AutoCloseable, Clearable {

    public static final String SIMULATE_INVALID_VERSION = "invalid-version";

    private final MapsContainer maps = new MapsContainer();
    private final Map<GetLabelRequest, GetLabelResponse> labels = maps.concurrentMap();
    private final Map<GetBuildRequest, GetBuildResponse> builds = maps.concurrentMap();
    private final Map<GetSchemaRequest, GetSchemaResponse> schemas = maps.concurrentMap();

    private final Map<String, TaskletExecutor<?, ?>> executors = maps.concurrentMap();
    private final Map<GetExecutionRequest, GetExecutionResponse> executions = maps.concurrentMap();

    private final TaskletV2ClientImpl impl = new TaskletV2ClientImpl();
    private final ExecutorService executor;

    private final TaskletServiceImplBase taskletService = new TaskletServiceImplBase() {
        @Override
        public void getLabel(GetLabelRequest request, StreamObserver<GetLabelResponse> responseObserver) {
            call(request, responseObserver, impl::getLabel);
        }

        @Override
        public void getBuild(GetBuildRequest request, StreamObserver<GetBuildResponse> responseObserver) {
            call(request, responseObserver, impl::getBuild);
        }

        @Override
        public void execute(ExecuteRequest request, StreamObserver<ExecuteResponse> responseObserver) {
            call(request, responseObserver, impl::execute);
        }

        @Override
        public void getExecution(GetExecutionRequest request, StreamObserver<GetExecutionResponse> responseObserver) {
            call(request, responseObserver, impl::getExecution);
        }
    };

    private final SchemaRegistryServiceImplBase schemaRegistryService = new SchemaRegistryServiceImplBase() {
        @Override
        public void getSchema(GetSchemaRequest request, StreamObserver<GetSchemaResponse> responseObserver) {
            call(request, responseObserver, impl::getSchema);
        }
    };

    private final Server server;
    private final Clock clock;

    public TaskletV2TestServer(String serverName, Clock clock, ExecutorService executor) {
        this.server = GrpcTestUtils.createAndStartServer(serverName, List.of(taskletService, schemaRegistryService));
        this.clock = clock;
        this.executor = executor;
    }

    @Override
    public void clear() {
        executors.values().stream()
                .map(TaskletExecutor::getStub)
                .forEach(TaskletContextStub::close);
        maps.clear();
    }

    public void registerLabel(String namespace, String tasklet, Label label) {
        var request = GetLabelRequest.newBuilder()
                .setNamespace(namespace)
                .setTasklet(tasklet)
                .setLabel(label.getMeta().getId())
                .build();
        log.info("Registered label: {}", TextFormat.shortDebugString(request));
        labels.put(
                request,
                GetLabelResponse.newBuilder()
                        .setLabel(label)
                        .build()
        );
    }

    public void registerLabel(String namespace, String tasklet, String label, String buildId) {
        registerLabel(namespace, tasklet,
                Label.newBuilder()
                        .setMeta(LabelMeta.newBuilder().setId(label))
                        .setSpec(LabelSpec.newBuilder().setBuildId(buildId))
                        .build()
        );
    }

    public void unregisterLabel(String namespace, String tasklet, String label) {
        var request = GetLabelRequest.newBuilder()
                .setNamespace(namespace)
                .setTasklet(tasklet)
                .setLabel(label)
                .build();
        if (labels.remove(request) != null) {
            log.info("Unregistered label: {}", TextFormat.shortDebugString(request));
        }
    }

    public void registerBuild(Build build) {
        var request = GetBuildRequest.newBuilder()
                .setBuildId(build.getMeta().getId())
                .build();
        log.info("Registered build: {}", TextFormat.shortDebugString(request));
        builds.put(
                request,
                GetBuildResponse.newBuilder()
                        .setBuild(build)
                        .build()
        );
    }

    public void registerBuild(String buildId, String schemaHash, String inputMessageType, String outputMessageType) {
        registerBuild(Build.newBuilder()
                .setMeta(BuildMeta.newBuilder()
                        .setId(buildId)
                        .setCreatedAt(ProtoConverter.convert(clock.instant()))
                        .setRevision(335)
                )
                .setSpec(BuildSpec.newBuilder()
                        .setSchema(IOSchema.newBuilder()
                                .setSimpleProto(IOSimpleSchemaProto.newBuilder()
                                        .setSchemaHash(schemaHash)
                                        .setInputMessage(inputMessageType)
                                        .setOutputMessage(outputMessageType)
                                )
                        )
                )
                .build());
    }

    public void updateBuildTimestamp(String buildId) {
        var buildResponse = builds.get(GetBuildRequest.newBuilder()
                .setBuildId(buildId)
                .build());
        Preconditions.checkState(buildResponse != null, "Build not registered: %s", buildId);

        var newBuilder = buildResponse.getBuild().toBuilder();
        newBuilder.getMetaBuilder().setCreatedAt(ProtoConverter.convert(clock.instant()));

        registerBuild(newBuilder.build());
    }

    public void registerSchema(String schemaHash, FileDescriptorSet files) {
        var request = GetSchemaRequest.newBuilder()
                .setHash(schemaHash)
                .build();
        log.info("Registered schema: {}", request);
        schemas.put(
                request,
                GetSchemaResponse.newBuilder()
                        .setSchema(files)
                        .build()
        );
    }

    public void registerSchema(String hash, String fileDescriptorsResource) {
        var descriptors = ProtobufTestUtils.parseProtoBinary(
                fileDescriptorsResource,
                FileDescriptorSet.class
        );
        registerSchema(hash, descriptors);
    }

    public <Input extends Message, Output extends Message> void registerExecutor(
            String buildId,
            Class<Input> inputType,
            Class<Output> outputType,
            TaskletAction<Input, Output> action
    ) {
        log.info("Registered executor: {}", buildId);

        var inputMessage = ProtoUtils.getMessageBuilder(inputType).build();
        var outputMessage = ProtoUtils.getMessageBuilder(outputType).build();
        var stub = TaskletContextStub.stub(inputType, outputType);
        var prev = executors.put(buildId, new TaskletExecutor<>(
                bytes -> {
                    var builder = inputMessage.newBuilderForType();
                    try {
                        builder.mergeFrom(bytes);
                    } catch (InvalidProtocolBufferException e) {
                        throw GrpcUtils.internalError("Unable to parse message as " +
                                inputType.getName() + ": " + e.getMessage());
                    }
                    //noinspection unchecked
                    return (Input) builder.build();
                },
                output -> {
                    Preconditions.checkState(Objects.equals(outputMessage.getClass(), output.getClass()));
                    return output.toByteString();
                },
                stub,
                action
        ));
        if (prev != null) {
            prev.getStub().close();
        }
    }

    @Override
    public void close() {
        server.shutdown();
    }

    private <Request, Response> void call(
            Request request,
            StreamObserver<Response> responseObserver,
            Function<Request, Response> impl
    ) {
        responseObserver.onNext(impl.apply(request));
        responseObserver.onCompleted();
    }

    private class TaskletV2ClientImpl implements TaskletV2Client, TaskletV2Client.AuthenticatedExecutor {

        @Override
        public GetLabelResponse getLabel(GetLabelRequest request) {
            return get(request, labels);
        }

        @Override
        public GetBuildResponse getBuild(GetBuildRequest request) {
            return get(request, builds);
        }

        @Override
        public GetSchemaResponse getSchema(GetSchemaRequest request) {
            return get(request, schemas);
        }

        @Override
        public AuthenticatedExecutor getExecutor(String oauthToken) {
            return this;
        }

        @Override
        public ExecuteResponse execute(ExecuteRequest request) {
            var label = getLabel(
                    GetLabelRequest.newBuilder()
                            .setNamespace(request.getNamespace())
                            .setTasklet(request.getTasklet())
                            .setLabel(request.getLabel())
                            .build()
            );
            var buildId = label.getLabel().getSpec().getBuildId();

            var build = getBuild(GetBuildRequest.newBuilder()
                    .setBuildId(buildId)
                    .build());

            // Just check if we have a schema
            // No need to actually load it
            getSchema(GetSchemaRequest.newBuilder()
                    .setHash(build.getBuild().getSpec().getSchema().getSimpleProto().getSchemaHash())
                    .build());

            var taskletExecutor = executors.get(buildId);
            if (taskletExecutor == null) {
                throw GrpcUtils.notFoundException("Executor not found for build " + buildId);
            }

            var id = UUID.randomUUID().toString();

            boolean simulateInvalid = request.getLabel().equals(SIMULATE_INVALID_VERSION);

            var execution = simulateInvalid
                    ? updateExecution(id, EExecutionStatus.E_EXECUTION_STATUS_INVALID, null)
                    : updateExecution(id, EExecutionStatus.E_EXECUTION_STATUS_EXECUTING, null);

            var response = ExecuteResponse.newBuilder()
                    .setExecution(execution)
                    .build();

            if (!simulateInvalid) {
                // Will be executed later
                executor.execute(() -> executeImpl(request, id, taskletExecutor));
            }
            return response;
        }

        @Override
        public GetExecutionResponse getExecution(GetExecutionRequest request) {
            return get(request, executions);
        }

        private <Input extends Message, Output extends Message> void executeImpl(
                ExecuteRequest request,
                String executionId,
                TaskletExecutor<Input, Output> taskletExecutor
        ) {
            try {
                var input = request.getInput().getSerializedData();
                var inputMessage = taskletExecutor.getInputParser().apply(input);

                var context = taskletExecutor.getStub().getContext();
                var outputMessage = taskletExecutor.getAction().execute(inputMessage, context);

                if (outputMessage.hasResult()) {
                    var output = taskletExecutor.getOutputParser().apply(outputMessage.getResult());
                    var result = ProcessingResult.newBuilder()
                            .setOutput(DataModel.ExecutionOutput.newBuilder()
                                    .setSerializedOutput(output))
                            .build();
                    updateExecution(executionId, EExecutionStatus.E_EXECUTION_STATUS_FINISHED, result);
                } else if (outputMessage.hasError()) {
                    var result = ProcessingResult.newBuilder()
                            .setUserError(outputMessage.getError())
                            .build();
                    updateExecution(executionId, EExecutionStatus.E_EXECUTION_STATUS_FINISHED, result);
                }
            } catch (Exception e) {
                var result = ProcessingResult.newBuilder()
                        .setServerError(DataModel.ServerError.newBuilder()
                                .setCode(DataModel.ErrorCodes.ErrorCode.ERROR_CODE_GENERIC)
                                .setDescription(e.getMessage())
                                .build())
                        .build();
                updateExecution(executionId, EExecutionStatus.E_EXECUTION_STATUS_FINISHED, result);
            }
        }

        private Execution updateExecution(
                String executionId,
                EExecutionStatus status,
                @Nullable ProcessingResult result
        ) {
            // Only necessary fields
            var executionBuilder = Execution.newBuilder();
            executionBuilder.getMetaBuilder()
                    .setId(executionId);
            executionBuilder.getStatusBuilder()
                    .setStatus(status);
            if (result != null) {
                executionBuilder.getStatusBuilder()
                        .setProcessingResult(result);
            }

            var execution = executionBuilder.build();
            executions.put(
                    GetExecutionRequest.newBuilder()
                            .setId(executionId)
                            .build(),
                    GetExecutionResponse.newBuilder()
                            .setExecution(execution)
                            .build()
            );
            return execution;
        }

        private <Request extends Message, Response> Response get(Request request, Map<Request, Response> map) {
            var response = map.get(request);
            if (response == null) {
                throw GrpcUtils.notFoundException(TextFormat.shortDebugString(request));
            }
            return response;
        }

        @Override
        public void close() throws Exception {
            //
        }
    }

}
