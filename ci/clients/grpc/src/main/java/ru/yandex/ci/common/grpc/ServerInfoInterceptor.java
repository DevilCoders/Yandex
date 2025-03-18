package ru.yandex.ci.common.grpc;

import java.net.InetAddress;
import java.net.UnknownHostException;

import io.grpc.ForwardingServerCall;
import io.grpc.Metadata;
import io.grpc.ServerCall;
import io.grpc.ServerCallHandler;
import io.grpc.ServerInterceptor;
import io.grpc.Status;

public class ServerInfoInterceptor implements ServerInterceptor {

    public static final Metadata.Key<String> HOST_HEADER = Metadata.Key.of(
            "host",
            Metadata.ASCII_STRING_MARSHALLER
    );

    public static final Metadata.Key<String> REQUEST_START_HEADER = Metadata.Key.of(
            "request-start-millis",
            Metadata.ASCII_STRING_MARSHALLER
    );

    public static final Metadata.Key<String> REQUEST_END_HEADER = Metadata.Key.of(
            "request-end-millis",
            Metadata.ASCII_STRING_MARSHALLER
    );

    public static final Metadata.Key<String> REQUEST_DURATION_HEADER = Metadata.Key.of(
            "request-duration-millis",
            Metadata.ASCII_STRING_MARSHALLER
    );


    private final String host;

    private ServerInfoInterceptor() {
        try {
            host = InetAddress.getLocalHost().getHostName();
        } catch (UnknownHostException e) {
            throw new RuntimeException(e);
        }

    }

    public static ServerInfoInterceptor instance() {
        return new ServerInfoInterceptor();
    }


    @Override
    public <ReqT, RespT> ServerCall.Listener<ReqT> interceptCall(
            ServerCall<ReqT, RespT> call, Metadata headers, ServerCallHandler<ReqT, RespT> next
    ) {
        long startMillis = System.currentTimeMillis();
        var interceptedCall = new ForwardingServerCall.SimpleForwardingServerCall<>(call) {
            @Override
            public void close(Status status, Metadata trailers) {
                long endMillis = System.currentTimeMillis();
                trailers.put(HOST_HEADER, host);
                trailers.put(REQUEST_START_HEADER, Long.toString(startMillis));
                trailers.put(REQUEST_END_HEADER, Long.toString(endMillis));
                trailers.put(REQUEST_DURATION_HEADER, Long.toString(endMillis - startMillis));
                super.close(status, trailers);
            }
        };

        return next.startCall(interceptedCall, headers);
    }
}
