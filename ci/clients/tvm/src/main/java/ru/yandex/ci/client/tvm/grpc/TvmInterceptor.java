package ru.yandex.ci.client.tvm.grpc;

import java.util.function.Function;
import java.util.function.Predicate;
import java.util.function.ToLongFunction;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import io.grpc.Metadata;
import io.grpc.MethodDescriptor;
import io.grpc.ServerCall;
import io.grpc.ServerCallHandler;
import io.grpc.ServerInterceptor;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.passport.tvmauth.TicketStatus;

import static ru.yandex.ci.client.tvm.grpc.YandexAuthInterceptor.AUTHENTICATED_USER;
import static ru.yandex.ci.client.tvm.grpc.YandexAuthInterceptor.unAuthenticated;
import static ru.yandex.ci.client.tvm.grpc.YandexAuthInterceptor.withContext;

@Slf4j
@RequiredArgsConstructor
final class TvmInterceptor<T> implements ServerInterceptor {

    @Nonnull
    private final Predicate<MethodDescriptor<?, ?>> isMandatory;
    @Nonnull
    private final Metadata.Key<String> ticketHeader;
    @Nonnull
    private final Function<String, String> getUserTicket;
    @Nonnull
    private final Function<String, T> getTicket;
    @Nonnull
    private final Function<T, TicketStatus> getTicketStatus;
    @Nonnull
    private final Function<T, String> geTicketDebugInfo;
    @Nonnull
    private final ToLongFunction<T> getTicketUid;

    @Override
    public <ReqT, RespT> ServerCall.Listener<ReqT> interceptCall(
            ServerCall<ReqT, RespT> call, Metadata headers, ServerCallHandler<ReqT, RespT> next) {
        if (YandexAuthInterceptor.UID_KEY.get() != null) {
            return next.startCall(call, headers);
        }
        if (AUTHENTICATED_USER.get() != null) {
            return next.startCall(call, headers); // Пользователь уже идентифицирован
        }

        var header = ticketHeader.name();
        var mandatory = isMandatory.test(call.getMethodDescriptor());

        @Nullable
        var ticketBody = headers.get(ticketHeader);
        if (ticketBody == null) {
            log.info("Header {} not found", header);
            return mandatory ?
                    unAuthenticated(call, headers, "Header " + header + " not found") :
                    next.startCall(call, headers);
        }

        @Nullable
        var ticket = getTicket.apply(ticketBody);
        if (ticket == null) {
            return mandatory ?
                    unAuthenticated(call, headers, "Cannot get ticket from header " + header) :
                    next.startCall(call, headers);
        }

        var status = getTicketStatus.apply(ticket);
        if (status != TicketStatus.OK) {
            log.info("Header {} ticket has invalid status {}", header, status);
            return mandatory ?
                    unAuthenticated(
                            call, headers, String.format("Invalid header %s. Status = %s, DebugInfo = %s",
                                    header, status, geTicketDebugInfo.apply(ticket))) :
                    next.startCall(call, headers);
        }

        var uid = getTicketUid.applyAsLong(ticket);
        var userTicket = getUserTicket.apply(ticketBody);
        return uid > 0 ?
                withContext(call, headers, next,
                        context -> context
                                .withValue(YandexAuthInterceptor.UID_KEY, uid)
                                .withValue(YandexAuthInterceptor.USER_TICKET, userTicket)) :
                next.startCall(call, headers);
    }

}
