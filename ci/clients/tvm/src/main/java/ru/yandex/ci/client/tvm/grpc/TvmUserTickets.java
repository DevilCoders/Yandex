package ru.yandex.ci.client.tvm.grpc;

import java.util.Arrays;
import java.util.function.Function;
import java.util.function.Predicate;

import javax.annotation.Nonnull;

import io.grpc.Metadata;
import io.grpc.MethodDescriptor;
import io.grpc.ServerCall;
import io.grpc.ServerCallHandler;
import io.grpc.ServerInterceptor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.client.tvm.TvmHeaders;
import ru.yandex.passport.tvmauth.CheckedUserTicket;
import ru.yandex.passport.tvmauth.TvmClient;

@Slf4j
class TvmUserTickets implements ServerInterceptor {

    static final Metadata.Key<String> TVM_USER_TICKET_HEADER =
            Metadata.Key.of(TvmHeaders.USER_TICKET, Metadata.ASCII_STRING_MARSHALLER);

    private final TvmInterceptor<CheckedUserTicket> delegate;

    TvmUserTickets(@Nonnull TvmClient tvmClient, Predicate<MethodDescriptor<?, ?>> mandatory) {
        this.delegate = new TvmInterceptor<>(
                mandatory,
                TVM_USER_TICKET_HEADER,
                Function.identity(),
                ticketBody -> {
                    try {
                        return tvmClient.checkUserTicket(ticketBody);
                    } catch (Exception e) {
                        log.error("Unexpected error when obtaining user ticket", e);
                        return null;
                    }
                },
                CheckedUserTicket::getStatus,
                CheckedUserTicket::debugInfo,
                ticket -> {
                    var uids = ticket.getUids();
                    log.info("{}, UIDS = {}", TVM_USER_TICKET_HEADER.name(), Arrays.toString(uids));
                    return uids.length > 0 ? uids[0] : 0;
                });
    }

    @Override
    public <ReqT, RespT> ServerCall.Listener<ReqT> interceptCall(
            ServerCall<ReqT, RespT> call, Metadata headers, ServerCallHandler<ReqT, RespT> next) {
        return delegate.interceptCall(call, headers, next);
    }
}
