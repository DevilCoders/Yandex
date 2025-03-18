package ru.yandex.ci.client.tvm.grpc;

import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.SocketAddress;
import java.util.ArrayList;
import java.util.Optional;
import java.util.function.Function;
import java.util.function.Predicate;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import io.grpc.Context;
import io.grpc.Contexts;
import io.grpc.Grpc;
import io.grpc.Metadata;
import io.grpc.MethodDescriptor;
import io.grpc.ServerCall;
import io.grpc.ServerCallHandler;
import io.grpc.ServerInterceptor;
import io.grpc.reflection.v1alpha.ServerReflectionGrpc;
import lombok.extern.slf4j.Slf4j;

@Slf4j
public class YandexAuthInterceptor implements ServerInterceptor {

    static final Context.Key<AuthenticatedUser> AUTHENTICATED_USER = Context.key("authenticatedUser");
    static final Context.Key<Long> UID_KEY = Context.key("uid");
    static final Context.Key<String> USER_TICKET = Context.key("user-ticket");

    private final ServerInterceptor[] chain;
    private final Predicate<MethodDescriptor<?, ?>> ignoreAuthMethods;

    public YandexAuthInterceptor(@Nonnull AuthSettings settings) {
        this.chain = getChain(settings);
        this.ignoreAuthMethods = method ->
                ServerReflectionGrpc.SERVICE_NAME.equals(method.getServiceName()) ||
                        settings.getIgnoreAuthMethods().test(method);
    }

    @Override
    public <ReqT, RespT> ServerCall.Listener<ReqT> interceptCall(
            ServerCall<ReqT, RespT> call, Metadata headers, ServerCallHandler<ReqT, RespT> next) {
        if (ignoreAuthMethods.test(call.getMethodDescriptor())) {
            log.info("Skip authorization for method {}", call.getMethodDescriptor().getFullMethodName());
            return next.startCall(call, headers);
        }

        return next(call, headers, next, 0);
    }

    private <ReqT, RespT> ServerCall.Listener<ReqT> next(
            ServerCall<ReqT, RespT> call, Metadata headers, ServerCallHandler<ReqT, RespT> next, int index) {
        if (index >= chain.length) {
            return next.startCall(call, headers);
        } else {
            return chain[index].interceptCall(call, headers, (nextCall, nextHeaders) ->
                    next(nextCall, nextHeaders, next, index + 1));
        }
    }

    static <ReqT, RespT> ServerCall.Listener<ReqT> unAuthenticated(
            ServerCall<ReqT, RespT> call, Metadata headers, String description) {
        log.warn("Unauthenticated: {}", description);
        call.close(io.grpc.Status.UNAUTHENTICATED.withDescription("Unauthenticated: " + description), headers);
        return new ServerCall.Listener<>() {
        };
    }

    static <ReqT, RespT> ServerCall.Listener<ReqT> withUser(ServerCall<ReqT, RespT> call, Metadata headers,
                                                            ServerCallHandler<ReqT, RespT> next,
                                                            AuthenticatedUser user) {
        return withContext(call, headers, next, context -> context.withValue(AUTHENTICATED_USER, user));
    }

    static <ReqT, RespT> ServerCall.Listener<ReqT> withContext(
            ServerCall<ReqT, RespT> call, Metadata headers, ServerCallHandler<ReqT, RespT> next,
            Function<Context, Context> withContext) {
        return Contexts.interceptCall(withContext.apply(Context.current()), call, headers, next);
    }

    static <ReqT, RespT> String getUserIp(ServerCall<ReqT, RespT> call) {
        return Optional.ofNullable(call.getAttributes().get(Grpc.TRANSPORT_ATTR_REMOTE_ADDR))
                .map(YandexAuthInterceptor::extractIpAddressFromGrpcRemoteAddr)
                .orElse("");
    }

    @Nullable
    private static String extractIpAddressFromGrpcRemoteAddr(@Nullable SocketAddress grpcAddr) {
        if (grpcAddr instanceof InetSocketAddress) {
            return removeIpv6ScopeId(((InetSocketAddress) grpcAddr).getAddress());
        } else {
            return null;
        }
    }

    private static String removeIpv6ScopeId(InetAddress address) {
        String hostAddress = address.getHostAddress();
        if (address instanceof Inet6Address) {
            // ipv6 может содержать scope, который не поддержан в ru.yandex.misc.ip.IpAddress
            // https://en.wikipedia.org/wiki/IPv6_address#Scoped_literal_IPv6_addresses
            if (hostAddress.indexOf('%') > -1) {
                return hostAddress.substring(0, hostAddress.indexOf('%'));
            }
        }
        return hostAddress;
    }

    static ServerInterceptor[] getChain(AuthSettings settings) {
        var tvmClient = settings.getTvmClient();
        var isDebug = settings.isDebug();

        var chain = new ArrayList<ServerInterceptor>();
        chain.add(new OAuthInterceptor(settings.getBlackbox(), settings.getOAuthScope()));

        if (isDebug) {
            chain.add(new TvmServiceTickets(tvmClient, AuthSettings.NOT_REQUIRED));
            chain.add(new TvmUserTickets(tvmClient, AuthSettings.NOT_REQUIRED));
            chain.add(new AuthenticatedWithDebug());
        } else {
            chain.add(new TvmServiceTickets(tvmClient, settings.getMandatoryServiceTicket()));
            chain.add(new TvmUserTickets(tvmClient, settings.getMandatoryUserTicket()));
        }
        chain.add(new AuthenticatedWithBlackbox(settings.getBlackbox()));
        return chain.toArray(ServerInterceptor[]::new);
    }

    public static Optional<AuthenticatedUser> getAuthenticatedUser() {
        return Optional.ofNullable(AUTHENTICATED_USER.get());
    }

}
