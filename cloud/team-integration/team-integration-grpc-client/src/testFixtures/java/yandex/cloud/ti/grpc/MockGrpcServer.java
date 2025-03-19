package yandex.cloud.ti.grpc;

import java.util.ArrayDeque;
import java.util.Queue;
import java.util.function.Supplier;

import io.grpc.ForwardingServerCallListener;
import io.grpc.Metadata;
import io.grpc.MethodDescriptor;
import io.grpc.ServerCall;
import io.grpc.ServerCallHandler;
import io.grpc.ServerMethodDefinition;
import io.grpc.ServerServiceDefinition;
import io.grpc.ServiceDescriptor;
import io.grpc.Status;
import io.grpc.stub.ServerCalls;
import io.grpc.stub.StreamObserver;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

public class MockGrpcServer {

    private final @NotNull Queue<ResponseHolder<?, ?>> responses = new ArrayDeque<>();
    private final @NotNull Queue<RequestHolder<?, ?>> requests = new ArrayDeque<>();


    public <ReqT, RespT> void enqueueResponse(
            @NotNull MethodDescriptor<ReqT, RespT> methodDescriptor,
            @NotNull Supplier<RespT> responseSupplier
    ) {
        responses.add(new ResponseHolder<>(
                methodDescriptor,
                responseSupplier
        ));
    }

    private <ReqT, RespT> void recordRequest(
            @NotNull MethodDescriptor<ReqT, RespT> methodDescriptor,
            @NotNull ReqT request,
            @NotNull Metadata headers
    ) {
        requests.add(new RequestHolder<>(
                methodDescriptor,
                request,
                headers
        ));
    }


    public @NotNull <ReqT, RespT> RequestHolder<ReqT, RespT> takeRequestHolder(
            @NotNull MethodDescriptor<ReqT, RespT> methodDescriptor
    ) {
        RequestHolder<?, ?> requestHolder = requests.remove();
        if (!methodDescriptor.equals(requestHolder.methodDescriptor())) {
            throw new IllegalStateException("expected %s, got %s with \n%s".formatted(
                    methodDescriptor.getFullMethodName(),
                    requestHolder.methodDescriptor().getFullMethodName(),
                    requestHolder.request()
            ));
        }
        return castRequestHolder(requestHolder);
    }

    @SuppressWarnings("unchecked")
    private static <ReqT, RespT> RequestHolder<ReqT, RespT> castRequestHolder(@NotNull RequestHolder<?, ?> requestHolder) {
        return (RequestHolder<ReqT, RespT>) requestHolder;
    }

    public <ReqT, RespT> ReqT takeRequest(
            @NotNull MethodDescriptor<ReqT, RespT> methodDescriptor
    ) {
        return takeRequestHolder(methodDescriptor).request();
    }


    private <ReqT, RespT> void call(
            @NotNull MethodDescriptor<ReqT, RespT> methodDescriptor,
            @NotNull ReqT request,
            @NotNull StreamObserver<RespT> responseObserver
    ) {
        Supplier<RespT> responseSupplier = getResponseSupplier(methodDescriptor, request);
        try {
            responseObserver.onNext(responseSupplier.get());
            responseObserver.onCompleted();
        } catch (RuntimeException e) {
            responseObserver.onError(e);
        }
    }

    private <ReqT, RespT> Supplier<RespT> getResponseSupplier(
            @NotNull MethodDescriptor<ReqT, RespT> methodDescriptor,
            @NotNull ReqT request
    ) {
        ResponseHolder<?, ?> responseHolder = responses.poll();
        if (responseHolder == null) {
            throw Status.INTERNAL
                    .withDescription("expected %s".formatted(
                            methodDescriptor.getFullMethodName()
                    ))
                    .asRuntimeException();
        }
        if (!methodDescriptor.equals(responseHolder.methodDescriptor())) {
            throw Status.INTERNAL
                    .withDescription("expected %s, got %s with\n%s".formatted(
                            methodDescriptor.getFullMethodName(),
                            responseHolder.methodDescriptor().getFullMethodName(),
                            request
                    ))
                    .asRuntimeException();
        }
        return castResponseSupplier(responseHolder.responseSupplier());
    }

    @SuppressWarnings("unchecked")
    private static <RespT> Supplier<RespT> castResponseSupplier(@NotNull Supplier<?> responseSupplier) {
        return (Supplier<RespT>) responseSupplier;
    }


    public ServerServiceDefinition mockService(
            @NotNull ServiceDescriptor serviceDescriptor
    ) {
        ServerServiceDefinition.Builder builder = ServerServiceDefinition.builder(serviceDescriptor);
        for (MethodDescriptor<?, ?> methodDescriptor : serviceDescriptor.getMethods()) {
            builder.addMethod(createServerMethodDefinition(methodDescriptor));
        }
        return builder.build();
    }

    private <ReqT, RespT> ServerMethodDefinition<ReqT, RespT> createServerMethodDefinition(
            @NotNull MethodDescriptor<ReqT, RespT> methodDescriptor
    ) {
        return ServerMethodDefinition.create(
                methodDescriptor,
                new MetadataCapturingServerCallHandler<>((request, responseObserver) -> call(
                        methodDescriptor,
                        request,
                        responseObserver
                ))
        );
    }


    private record ResponseHolder<ReqT, RespT>(
            @NotNull MethodDescriptor<ReqT, RespT> methodDescriptor,
            @NotNull Supplier<RespT> responseSupplier
    ) {}

    public record RequestHolder<ReqT, RespT>(
            @NotNull MethodDescriptor<ReqT, RespT> methodDescriptor,
            @NotNull ReqT request,
            @NotNull Metadata headers
    ) {}


    private class MetadataCapturingServerCallHandler<ReqT, RespT> implements ServerCallHandler<ReqT, RespT> {

        private final @NotNull ServerCallHandler<ReqT, RespT> handler;

        MetadataCapturingServerCallHandler(@NotNull ServerCalls.UnaryMethod<ReqT, RespT> method) {
            handler = ServerCalls.asyncUnaryCall(method);
        }

        @Override
        public ServerCall.Listener<ReqT> startCall(ServerCall<ReqT, RespT> call, Metadata headers) {
            return new MetadataCapturingServerCallListener<>(
                    handler.startCall(call, headers),
                    call.getMethodDescriptor(),
                    headers
            );
        }

    }

    private class MetadataCapturingServerCallListener<ReqT, RespT> extends ForwardingServerCallListener.SimpleForwardingServerCallListener<ReqT> {

        private final @NotNull MethodDescriptor<ReqT, RespT> methodDescriptor;
        private final @NotNull Metadata headers;

        MetadataCapturingServerCallListener(
                @NotNull ServerCall.Listener<ReqT> delegate,
                @NotNull MethodDescriptor<ReqT, RespT> methodDescriptor,
                @Nullable Metadata headers
        ) {
            super(delegate);
            this.methodDescriptor = methodDescriptor;
            this.headers = headers != null ? headers : new Metadata();
        }

        @Override
        public void onMessage(ReqT message) {
            recordRequest(methodDescriptor, message, headers);
            super.onMessage(message);
        }

    }

}
