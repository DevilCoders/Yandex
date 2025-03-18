package ru.yandex.ci.client.tvm.grpc;

import java.util.function.Predicate;

import javax.annotation.Nonnull;

import io.grpc.Metadata;
import io.grpc.MethodDescriptor;
import io.grpc.ServerCall;
import io.grpc.ServerCallHandler;
import io.grpc.ServerInterceptor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.client.tvm.TvmHeaders;
import ru.yandex.passport.tvmauth.CheckedServiceTicket;
import ru.yandex.passport.tvmauth.TvmClient;

@Slf4j
class TvmServiceTickets implements ServerInterceptor {

    static final Metadata.Key<String> TVM_SERVICE_TICKET_HEADER =
            Metadata.Key.of(TvmHeaders.SERVICE_TICKET, Metadata.ASCII_STRING_MARSHALLER);

    private final TvmInterceptor<CheckedServiceTicket> delegate;

    TvmServiceTickets(@Nonnull TvmClient tvmClient, Predicate<MethodDescriptor<?, ?>> mandatory) {
        this.delegate = new TvmInterceptor<>(
                mandatory,
                TVM_SERVICE_TICKET_HEADER,
                ticketBody -> null,
                ticketBody -> {
                    try {
                        return tvmClient.checkServiceTicket(ticketBody);
                    } catch (Exception e) {
                        log.error("Unexpected error when obtaining service ticket", e);
                        return null;
                    }
                },
                CheckedServiceTicket::getStatus,
                CheckedServiceTicket::debugInfo,
                ticket -> {
                    var sid = ticket.getSrc();
                    var uid = ticket.getIssuerUid();

                    log.info("{}, SID = {}, UID = {}", TVM_SERVICE_TICKET_HEADER.name(), sid, uid);
                    return uid;
                });
    }

    @Override
    public <ReqT, RespT> ServerCall.Listener<ReqT> interceptCall(
            ServerCall<ReqT, RespT> call, Metadata headers, ServerCallHandler<ReqT, RespT> next) {
        return delegate.interceptCall(call, headers, next);
    }

}
