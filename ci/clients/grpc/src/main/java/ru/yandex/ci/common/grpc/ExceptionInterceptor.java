package ru.yandex.ci.common.grpc;

import java.util.concurrent.ExecutionException;

import javax.annotation.Nullable;

import com.google.common.util.concurrent.MoreExecutors;
import com.google.common.util.concurrent.SettableFuture;
import io.grpc.Attributes;
import io.grpc.ForwardingServerCall;
import io.grpc.ForwardingServerCallListener;
import io.grpc.Metadata;
import io.grpc.ServerCall;
import io.grpc.ServerCallHandler;
import io.grpc.ServerInterceptor;
import io.grpc.Status;
import io.grpc.StatusRuntimeException;
import io.grpc.internal.SerializingExecutor;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.ci.common.exceptions.CustomExceptionConverter;
import ru.yandex.ci.common.exceptions.DefaultExceptionConverter;


/**
 * Класс является почти полной копией io.grpc.util.TransmitStatusRuntimeExceptionInterceptor,
 * с тем исключением, что он обрабатывает любые ошибки, а не только  StatusRuntimeException
 */
public class ExceptionInterceptor implements ServerInterceptor {
    private static final Logger log = LoggerFactory.getLogger(ServerInterceptor.class);

    private final CustomExceptionConverter customExceptionConverter;

    private ExceptionInterceptor(CustomExceptionConverter customExceptionConverter) {
        this.customExceptionConverter = customExceptionConverter;
    }

    public static ExceptionInterceptor instance() {
        return new ExceptionInterceptor(DefaultExceptionConverter.instance());
    }

    public static ExceptionInterceptor instance(CustomExceptionConverter customExceptionConverter) {
        return new ExceptionInterceptor(customExceptionConverter);
    }

    @Override
    public <ReqT, RespT> ServerCall.Listener<ReqT> interceptCall(
            ServerCall<ReqT, RespT> call, Metadata headers, ServerCallHandler<ReqT, RespT> next) {
        final ServerCall<ReqT, RespT> serverCall = new SerializingServerCall<>(call);
        ServerCall.Listener<ReqT> listener = next.startCall(serverCall, headers);
        return new ForwardingServerCallListener.SimpleForwardingServerCallListener<>(listener) {
            @Override
            public void onMessage(ReqT message) {
                try {
                    super.onMessage(message);
                } catch (Exception e) {
                    closeWithException(e);
                }
            }

            @Override
            public void onHalfClose() {
                try {
                    super.onHalfClose();
                } catch (Exception e) {
                    closeWithException(e);
                }
            }

            @Override
            public void onCancel() {
                try {
                    super.onCancel();
                } catch (Exception e) {
                    closeWithException(e);
                }
            }

            @Override
            public void onComplete() {
                try {
                    super.onComplete();
                } catch (Exception e) {
                    closeWithException(e);
                }
            }

            @Override
            public void onReady() {
                try {
                    super.onReady();
                } catch (Exception e) {
                    closeWithException(e);
                }
            }

            private void closeWithException(Exception e) {
                if (e instanceof StatusRuntimeException) {
                    log.warn("Error when processing GRPC call", e);
                    closeWithStatusRuntimeException((StatusRuntimeException) e);
                    return;
                }

                var status = ExceptionInterceptor.this.customExceptionConverter.convert(e);
                if (status.isPresent()) {
                    log.warn("Error when processing GRPC call", e);
                    serverCall.close(status.get(), new Metadata());
                    return;
                }

                log.error("Unhandled error when processing GRPC call", e);
                serverCall.close(Status.INTERNAL.withDescription(e.getMessage()).withCause(e), new Metadata());
            }

            private void closeWithStatusRuntimeException(StatusRuntimeException e) {
                Metadata metadata = e.getTrailers();
                if (metadata == null) {
                    metadata = new Metadata();
                }
                serverCall.close(e.getStatus(), metadata);
            }

        };
    }


    /**
     * A {@link ServerCall} that wraps around a non thread safe delegate and provides thread safe
     * access by serializing everything on an executor.
     */
    private static class SerializingServerCall<ReqT, RespT> extends
            ForwardingServerCall.SimpleForwardingServerCall<ReqT, RespT> {
        private static final String ERROR_MSG = "Encountered error during serialized access";
        private final SerializingExecutor serializingExecutor =
                new SerializingExecutor(MoreExecutors.directExecutor());
        private boolean closeCalled = false;

        SerializingServerCall(ServerCall<ReqT, RespT> delegate) {
            super(delegate);
        }

        @Override
        public void sendMessage(final RespT message) {
            serializingExecutor.execute(() ->
                    SerializingServerCall.super.sendMessage(message));
        }

        @Override
        public void request(final int numMessages) {
            serializingExecutor.execute(() ->
                    SerializingServerCall.super.request(numMessages));
        }

        @Override
        public void sendHeaders(final Metadata headers) {
            serializingExecutor.execute(() ->
                    SerializingServerCall.super.sendHeaders(headers));
        }

        @Override
        public void close(final Status status, final Metadata trailers) {
            serializingExecutor.execute(() -> {
                if (!closeCalled) {
                    closeCalled = true;

                    SerializingServerCall.super.close(status, trailers);
                }
            });
        }

        @Override
        public boolean isReady() {
            final SettableFuture<Boolean> retVal = SettableFuture.create();
            serializingExecutor.execute(() ->
                    retVal.set(SerializingServerCall.super.isReady()));
            try {
                return retVal.get();
            } catch (InterruptedException | ExecutionException e) {
                throw new RuntimeException(ERROR_MSG, e);
            }
        }

        @Override
        public boolean isCancelled() {
            final SettableFuture<Boolean> retVal = SettableFuture.create();
            serializingExecutor.execute(() ->
                    retVal.set(SerializingServerCall.super.isCancelled()));
            try {
                return retVal.get();
            } catch (InterruptedException | ExecutionException e) {
                throw new RuntimeException(ERROR_MSG, e);
            }
        }

        @Override
        public void setMessageCompression(final boolean enabled) {
            serializingExecutor.execute(() ->
                    SerializingServerCall.super.setMessageCompression(enabled));
        }

        @Override
        public void setCompression(final String compressor) {
            serializingExecutor.execute(() ->
                    SerializingServerCall.super.setCompression(compressor));
        }

        @Override
        public Attributes getAttributes() {
            final SettableFuture<Attributes> retVal = SettableFuture.create();
            serializingExecutor.execute(() ->
                    retVal.set(SerializingServerCall.super.getAttributes()));
            try {
                return retVal.get();
            } catch (InterruptedException | ExecutionException e) {
                throw new RuntimeException(ERROR_MSG, e);
            }
        }

        @Nullable
        @Override
        public String getAuthority() {
            final SettableFuture<String> retVal = SettableFuture.create();
            serializingExecutor.execute(() ->
                    retVal.set(SerializingServerCall.super.getAuthority()));
            try {
                return retVal.get();
            } catch (InterruptedException | ExecutionException e) {
                throw new RuntimeException(ERROR_MSG, e);
            }
        }
    }
}
