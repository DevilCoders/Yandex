package ru.yandex.ci.common.grpc;

import java.util.Set;
import java.util.UUID;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.MessageOrBuilder;
import com.google.protobuf.util.JsonFormat;
import io.grpc.ForwardingServerCall;
import io.grpc.ForwardingServerCallListener;
import io.grpc.Metadata;
import io.grpc.MethodDescriptor;
import io.grpc.ServerCall;
import io.grpc.ServerCallHandler;
import io.grpc.ServerInterceptor;
import io.grpc.Status;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.slf4j.MDC;

public class ServerLoggingInterceptor implements ServerInterceptor {
    private static final Logger log = LoggerFactory.getLogger(ServerLoggingInterceptor.class);
    private static final Logger GRPC_CALLS = LoggerFactory.getLogger("GRPC_CALLS");
    private static final String REQUEST_ID_MDC = "request-id";

    private static final Metadata.Key<String> REQUEST_ID_KEY = Metadata.Key.of(
            "X-Request-Id",
            Metadata.ASCII_STRING_MARSHALLER
    );

    private final Set<String> ignoreLoggingMethods;

    private final JsonFormat.Printer logPrinter = JsonFormat.printer()
            .omittingInsignificantWhitespace()
            .preservingProtoFieldNames();

    public ServerLoggingInterceptor(MethodDescriptor<?, ?>... ignoreLoggingMethods) {
        this.ignoreLoggingMethods = Stream.of(ignoreLoggingMethods)
                .map(MethodDescriptor::getFullMethodName)
                .collect(Collectors.toUnmodifiableSet());
    }

    @Override
    public <ReqT, RespT> ServerCall.Listener<ReqT> interceptCall(
            ServerCall<ReqT, RespT> call, Metadata headers, ServerCallHandler<ReqT, RespT> next) {
        final String methodName = call.getMethodDescriptor().getFullMethodName();
        if (ignoreLoggingMethods.contains(methodName)) {
            return next.startCall(call, headers);
        }

        String requestId = headers.containsKey(REQUEST_ID_KEY)
                ? headers.get(REQUEST_ID_KEY)
                : "ci-random:" + UUID.randomUUID();

        long started = System.currentTimeMillis();
        try (var ignored = requestContext(requestId)) {
            log.info("{} start", methodName);

            var interceptedCall = new ForwardingServerCall.SimpleForwardingServerCall<>(call) {
                @Override
                public void sendMessage(RespT message) {
                    try (var ignored = requestContext(requestId)) {
                        logTrace("<<<", methodName, message);
                        super.sendMessage(message);
                    }
                }

                @Override
                public void close(Status status, Metadata trailers) {
                    try (var ignored = requestContext(requestId)) {
                        long durationMs = System.currentTimeMillis() - started;

                        StringBuilder sb = new StringBuilder();
                        sb.append(methodName).append(" ").append(status.getCode());
                        sb.append(" in ").append(durationMs).append("ms");
                        if (status.getDescription() != null) {
                            sb.append(": ").append(status.getDescription());
                        }
                        if (!trailers.keys().isEmpty()) {
                            sb.append(", ").append(trailers);
                        }
                        String message = sb.toString();

                        if (!status.isOk()) {
                            if (status.getCause() != null) {
                                log.error(message, status.getCause());
                            } else {
                                log.warn(message);
                            }
                        } else {
                            log.info(message);
                        }

                        super.close(status, trailers);
                    }
                }
            };

            return new ForwardingServerCallListener.SimpleForwardingServerCallListener<>(
                    next.startCall(interceptedCall, headers)) {

                @Override
                public void onMessage(ReqT message) {
                    try (var ignored = requestContext(requestId)) {
                        logTrace(">>>", methodName, message);
                        super.onMessage(message);
                    }
                }

                @Override
                public void onHalfClose() {
                    try (var ignored = requestContext(requestId)) {
                        super.onHalfClose();
                    }
                }

                @Override
                public void onCancel() {
                    try (var ignored = requestContext(requestId)) {
                        super.onCancel();
                    }
                }

                @Override
                public void onComplete() {
                    try (var ignored = requestContext(requestId)) {
                        super.onComplete();
                    }
                }

                @Override
                public void onReady() {
                    try (var ignored = requestContext(requestId)) {
                        super.onReady();
                    }
                }
            };
        }
    }

    private void logTrace(String direction, String methodName, Object message) {
        if (GRPC_CALLS.isTraceEnabled()) {
            try {
                GRPC_CALLS.trace("{} {} {}", direction, methodName, logPrinter.print((MessageOrBuilder) message));
            } catch (InvalidProtocolBufferException | ClassCastException e) {
                GRPC_CALLS.warn("{} {} cannot serialize message to json: {}", direction, methodName, message);
            }
        }
    }

    private MDC.MDCCloseable requestContext(String requestId) {
        return MDC.putCloseable(REQUEST_ID_MDC, requestId);
    }

}
