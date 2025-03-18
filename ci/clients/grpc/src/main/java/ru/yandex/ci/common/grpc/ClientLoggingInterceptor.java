package ru.yandex.ci.common.grpc;

import java.util.UUID;
import java.util.function.Predicate;

import javax.annotation.Nonnull;

import com.google.protobuf.MessageOrBuilder;
import com.google.protobuf.TextFormat;
import io.grpc.CallOptions;
import io.grpc.Channel;
import io.grpc.ClientCall;
import io.grpc.ClientInterceptor;
import io.grpc.Metadata;
import io.grpc.MethodDescriptor;
import io.grpc.Status;
import io.grpc.protobuf.StatusProto;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

@Slf4j
@RequiredArgsConstructor
public class ClientLoggingInterceptor implements ClientInterceptor {

    private static final Metadata.Key<String> REQUEST_ID_KEY = Metadata.Key.of(
            "X-Request-Id",
            Metadata.ASCII_STRING_MARSHALLER
    );

    @Nonnull
    private final String clientName;
    @Nonnull
    private final Predicate<MethodDescriptor<?, ?>> logFullResponsePredicate;
    @Nonnull
    private final Predicate<MethodDescriptor<?, ?>> logEnabledPredicate;

    @Override
    public <M, R> ClientCall<M, R> interceptCall(MethodDescriptor<M, R> method, CallOptions callOptions, Channel next) {
        return new BackendForwardingClientCall<>(
                method,
                next.newCall(method, callOptions),
                clientName,
                logFullResponsePredicate,
                logEnabledPredicate
        );
    }

    @RequiredArgsConstructor
    private static class ResponseListener<R> extends ClientCall.Listener<R> {

        private final String clientName;
        private final String methodName;
        private final ClientCall.Listener<R> responseListener;
        private final boolean logFullResponse;
        private final boolean logEnabled;
        private final boolean serverSendsOneMessage;
        private final String requestId;
        private final long started;

        private int messageNumber = 0;

        @Override
        public void onHeaders(Metadata headers) {
            responseListener.onHeaders(headers);
        }

        @Override
        public void onMessage(R message) {
            if (logEnabled) {
                messageNumber++;
                if (logFullResponse) {
                    var messageString = TextFormat.shortDebugString((MessageOrBuilder) message);
                    log.info("[{}], Received GRPC [{}], [{}={}], response = [{}]",
                            clientName, methodName, REQUEST_ID_KEY.name(), requestId, messageString);
                } else {
                    if (++messageNumber % 100 == 0 || serverSendsOneMessage) {
                        log.info("[{}], Received GRPC [{}], [{}={}], message number {}",
                                clientName, methodName, REQUEST_ID_KEY.name(), requestId, messageNumber);

                    }
                }
            }

            responseListener.onMessage(message);
        }

        @Override
        public void onClose(Status status, Metadata trailers) {
            if (logEnabled) {
                var elapsed = System.currentTimeMillis() - started;
                if (status.getCode() != Status.Code.OK) {
                    var statusProto = StatusProto.fromStatusAndTrailers(status, trailers);
                    var statusString = TextFormat.shortDebugString(statusProto);
                    log.warn("[{}], Complete GRPC [{}], [{}={}] took {} msec with error status = [{}], " +
                                    "description = [{}]",
                            clientName, methodName, REQUEST_ID_KEY.name(), requestId, elapsed, status, statusString);
                } else {
                    log.warn("[{}], Complete GRPC [{}], [{}={}] took {} msec",
                            clientName, methodName, REQUEST_ID_KEY.name(), requestId, elapsed);
                }
            }

            responseListener.onClose(status, trailers);
        }

        @Override
        public void onReady() {
            responseListener.onReady();
        }
    }

    private static class BackendForwardingClientCall<M, R>
            extends io.grpc.ForwardingClientCall.SimpleForwardingClientCall<M, R> {

        private final MethodDescriptor<M, R> method;
        private final String clientName;
        private final String methodName;
        private final String requestId;
        private final boolean logFullResponse;
        private final boolean logEnabled;

        protected BackendForwardingClientCall(
                MethodDescriptor<M, R> method,
                ClientCall<M, R> delegate,
                String clientName,
                Predicate<MethodDescriptor<?, ?>> logFullResponsePredicate,
                Predicate<MethodDescriptor<?, ?>> logEnabledPredicate
        ) {
            super(delegate);
            this.method = method;
            this.methodName = method.getFullMethodName();
            this.clientName = clientName;
            this.requestId = UUID.randomUUID().toString();
            this.logFullResponse = logFullResponsePredicate.test(method);
            this.logEnabled = logEnabledPredicate.test(method);
        }

        @Override
        public void sendMessage(M message) {
            if (logEnabled) {
                var messageString = TextFormat.shortDebugString((MessageOrBuilder) message);
                log.info("[{}],  Sending GRPC [{}], [{}={}], request = [{}]",
                        clientName, methodName, REQUEST_ID_KEY.name(), requestId, messageString);
            }
            super.sendMessage(message);
        }

        @Override
        public void start(Listener<R> responseListener, Metadata headers) {
            headers.put(REQUEST_ID_KEY, requestId);
            var listener = new ResponseListener<>(
                    clientName, methodName, responseListener,
                    logFullResponse, logEnabled, method.getType().serverSendsOneMessage(),
                    requestId, System.currentTimeMillis()
            );
            super.start(listener, headers);
        }
    }
}
