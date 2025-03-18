package ru.yandex.ci.client.tvm.grpc;

import java.io.IOException;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.UnknownHostException;
import java.util.Random;
import java.util.concurrent.Executor;
import java.util.concurrent.TimeUnit;
import java.util.function.Consumer;

import javax.annotation.Nullable;

import com.google.protobuf.Empty;
import io.grpc.CallCredentials;
import io.grpc.Grpc;
import io.grpc.ManagedChannel;
import io.grpc.Metadata;
import io.grpc.Server;
import io.grpc.StatusRuntimeException;
import io.grpc.inprocess.InProcessChannelBuilder;
import io.grpc.inprocess.InProcessServerBuilder;
import io.grpc.stub.StreamObserver;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.Timeout;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.Mock;
import org.mockito.junit.jupiter.MockitoExtension;

import ru.yandex.ci.api.internal.InternalApiGrpc;
import ru.yandex.ci.api.internal.InternalApiOuterClass;
import ru.yandex.ci.client.blackbox.BlackboxClient;
import ru.yandex.ci.client.blackbox.OAuthResponse;
import ru.yandex.ci.client.blackbox.OAuthResponse.OAuthInfo;
import ru.yandex.ci.client.blackbox.UserInfoResponse;
import ru.yandex.ci.util.CollectionUtils;
import ru.yandex.passport.tvmauth.CheckedServiceTicket;
import ru.yandex.passport.tvmauth.CheckedUserTicket;
import ru.yandex.passport.tvmauth.TicketStatus;
import ru.yandex.passport.tvmauth.TvmClient;
import ru.yandex.passport.tvmauth.Unittest;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.when;

@SuppressWarnings("ResultOfMethodCallIgnored")
@ExtendWith(MockitoExtension.class)
@Timeout(value = 5)
class YandexAuthInterceptorTest {
    private static final String USER_TICKET = "unit-test-user-ticket";
    private static final String SERVICE_TICKET = "unit-test-service-ticket";
    private static final int SERVICE_ID = 2018960;
    private static final long USER_UID = 63393562;
    private static final String IP_ADDRESS = "10.15.0.1";

    @Mock
    private TvmClient tvmClient;
    @Mock
    private BlackboxClient blackboxClient;

    private InternalApiGrpc.InternalApiBlockingStub pingService;
    @Nullable
    private ManagedChannel channel;
    @Nullable
    private Server mockServer;
    private CallAttributesInterceptor attributesInterceptor;

    private YandexAuthInterceptor yandexAuthInterceptor;

    @Nullable
    private AuthenticatedUser authenticatedUser;
    @Nullable
    private InternalApiGrpc.InternalApiImplBase serviceHandler;

    @BeforeEach
    public void setUp() throws IOException, InterruptedException {
        setUpAuth(init -> init
                .mandatoryUserTicket(AuthSettings.REQUIRED)
                .mandatoryServiceTicket(AuthSettings.NOT_REQUIRED));
    }

    @AfterEach
    public void tearDown() throws InterruptedException {
        this.tearDownAuth();
    }

    @Test
    public void testWithUserTicketOnly() {
        when(tvmClient.checkUserTicket(USER_TICKET))
                .thenReturn(userTicketWith(TicketStatus.OK));

        String login = "USERNAME";

        whenBlackBoxUserInfoThenReturn(login);

        pingService.withCallCredentials(withUserTicket())
                .ping(Empty.getDefaultInstance());

        assertThat(authenticatedUser).isNotNull();
        assertThat(authenticatedUser.getLogin()).isEqualTo(login);
        assertThat(authenticatedUser.getTvmUserTicket()).isEqualTo(USER_TICKET);
    }

    @Test
    public void testWithUserAndServiceTickets() {
        when(tvmClient.checkUserTicket(USER_TICKET))
                .thenReturn(userTicketWith(TicketStatus.OK));

        when(tvmClient.checkServiceTicket(SERVICE_TICKET))
                .thenReturn(serviceTicketWith(TicketStatus.OK));

        String login = "USERNAME";

        whenBlackBoxUserInfoThenReturn(login);

        pingService.withCallCredentials(withUserAndServiceTicket())
                .ping(Empty.getDefaultInstance());


        assertThat(authenticatedUser).isNotNull();
        assertThat(authenticatedUser.getLogin()).isEqualTo(login);
        assertThat(authenticatedUser.getTvmUserTicket()).isEqualTo(USER_TICKET);
    }

    @Test
    public void testWithUserAndServiceTickets_withOAuthEnabled() throws Exception {
        setUpAuth(init -> init.oAuthScope("ci:api"));

        when(tvmClient.checkUserTicket(USER_TICKET))
                .thenReturn(userTicketWith(TicketStatus.OK));

        when(tvmClient.checkServiceTicket(SERVICE_TICKET))
                .thenReturn(serviceTicketWith(TicketStatus.OK));

        String login = "USERNAME";

        whenBlackBoxUserInfoThenReturn(login);

        pingService.withCallCredentials(withUserAndServiceTicket())
                .ping(Empty.getDefaultInstance());


        assertThat(authenticatedUser).isNotNull();
        assertThat(authenticatedUser.getLogin()).isEqualTo(login);
        assertThat(authenticatedUser.getTvmUserTicket()).isEqualTo(USER_TICKET);
    }

    @Test
    public void testWithServiceTicketAndUid() {
        when(tvmClient.checkServiceTicket(SERVICE_TICKET))
                .thenReturn(serviceTicketWith(TicketStatus.OK, 1));

        String login = "USERNAME";

        whenBlackBoxUserInfoThenReturn(login);

        pingService.withCallCredentials(withServiceTicket())
                .ping(Empty.getDefaultInstance());

        assertThat(authenticatedUser).isNotNull();
        assertThat(authenticatedUser.getLogin()).isEqualTo(login);
        assertThat(authenticatedUser.getTvmUserTicket()).isNull();
    }

    @Test
    public void testWithServiceTicketOnly() throws IOException, InterruptedException {
        setUpAuth(init -> init
                .mandatoryUserTicket(AuthSettings.NOT_REQUIRED)
                .mandatoryServiceTicket(AuthSettings.REQUIRED));

        when(tvmClient.checkServiceTicket(SERVICE_TICKET))
                .thenReturn(serviceTicketWith(TicketStatus.OK)); // Неважно, какой это тикет (uid-а может и не быть)


        pingService.withCallCredentials(withServiceTicket())
                .ping(Empty.getDefaultInstance());

        assertThat(authenticatedUser).isNull();
    }

    @Test
    public void testWithoutUserTicketInDebug() throws IOException, InterruptedException {
        setUpAuth(init -> init
                .mandatoryUserTicket(AuthSettings.REQUIRED)
                .mandatoryServiceTicket(AuthSettings.NOT_REQUIRED)
                .debug(true));

        pingService.ping(Empty.getDefaultInstance());

        assertThat(authenticatedUser).isNotNull();
        assertThat(authenticatedUser.getLogin()).isEqualTo("user42");
        assertThat(authenticatedUser.getTvmUserTicket()).isEqualTo("debug");
    }

    @Test
    public void testWithInvalidUserTicketDefault() {
        when(tvmClient.checkUserTicket(USER_TICKET))
                .thenReturn(userTicketWith(TicketStatus.EXPIRED));

        assertThatThrownBy(() ->
                pingService.withCallCredentials(withUserTicket())
                        .ping(Empty.getDefaultInstance())
        )
                .isInstanceOf(StatusRuntimeException.class)
                .satisfies(exception -> {
                    var statusException = ((StatusRuntimeException) exception);
                    assertThat(statusException.getStatus().getCode()).isEqualTo(io.grpc.Status.Code.UNAUTHENTICATED);
                })
                .hasMessageContaining("Unauthenticated: Invalid header x-ya-user-ticket. Status = EXPIRED");
    }

    @Test
    public void testWithInvalidServiceTicketDefault() {
        when(tvmClient.checkServiceTicket(SERVICE_TICKET))
                .thenReturn(serviceTicketWith(TicketStatus.EXPIRED));

        assertThatThrownBy(() ->
                pingService.withCallCredentials(withServiceTicket())
                        .ping(Empty.getDefaultInstance())
        )
                .isInstanceOf(StatusRuntimeException.class)
                .satisfies(exception -> {
                    var statusException = ((StatusRuntimeException) exception);
                    assertThat(statusException.getStatus().getCode()).isEqualTo(io.grpc.Status.Code.UNAUTHENTICATED);
                })
                .hasMessageContaining("Unauthenticated: Header x-ya-user-ticket not found");
    }

    @Test
    public void testWithoutMandatoryUserTicket() {
        when(tvmClient.checkServiceTicket(SERVICE_TICKET))
                .thenReturn(serviceTicketWith(TicketStatus.OK)); // no uid

        assertThatThrownBy(() ->
                pingService.withCallCredentials(withServiceTicket())
                        .ping(Empty.getDefaultInstance())
        )
                .isInstanceOf(StatusRuntimeException.class)
                .satisfies(exception -> {
                    var statusException = ((StatusRuntimeException) exception);
                    assertThat(statusException.getStatus().getCode()).isEqualTo(io.grpc.Status.Code.UNAUTHENTICATED);
                })
                .hasMessageContaining("Unauthenticated: Header x-ya-user-ticket not found");
    }

    @Test
    public void testWithoutBlackboxResponse() {
        when(tvmClient.checkUserTicket(USER_TICKET))
                .thenReturn(userTicketWith(TicketStatus.OK));

        // No blackbox response
        var response = new UserInfoResponse(null);
        whenBlackBoxUserInfoThenReturn(response);

        assertThatThrownBy(() ->
                pingService.withCallCredentials(withUserTicket())
                        .ping(Empty.getDefaultInstance())
        )
                .isInstanceOf(StatusRuntimeException.class)
                .satisfies(exception -> {
                    var statusException = ((StatusRuntimeException) exception);
                    assertThat(statusException.getStatus().getCode()).isEqualTo(io.grpc.Status.Code.UNAUTHENTICATED);
                })
                .hasMessageContaining("Unauthenticated: Unable to find login for user %s and IpAddress %s"
                        .formatted(USER_UID, IP_ADDRESS));
    }

    @Test
    public void withoutMandatoryServiceTicket() throws IOException, InterruptedException {
        setUpAuth(init -> init
                .mandatoryUserTicket(AuthSettings.NOT_REQUIRED)
                .mandatoryServiceTicket(AuthSettings.REQUIRED));

        assertThatThrownBy(() ->
                pingService.withCallCredentials(withUserTicket())
                        .ping(Empty.getDefaultInstance())
        )
                .isInstanceOf(StatusRuntimeException.class)
                .satisfies(exception -> {
                    var statusException = ((StatusRuntimeException) exception);
                    assertThat(statusException.getStatus().getCode()).isEqualTo(io.grpc.Status.Code.UNAUTHENTICATED);
                })
                .hasMessageContaining("Unauthenticated: Header x-ya-service-ticket not found");
    }

    @Test
    public void testOAuthValid() throws Exception {
        String scope = "ci:api";
        String token = "AQAD-ABCD42";
        setUpAuth(init -> init.oAuthScope(scope));


        var response = new OAuthResponse("robot-ci", new OAuthInfo(CollectionUtils.linkedSet("d:e", scope, "a:b")));

        whenBlackBoxOAuthThenReturn(response, token);

        pingService.withCallCredentials(new OAuthCallCredentials(token))
                .ping(Empty.getDefaultInstance());

    }

    @Test
    public void testOAuthInvalidScope() throws Exception {
        String token = "AQAD-ABCD42";
        setUpAuth(init -> init.oAuthScope("ci:api"));

        var response = new OAuthResponse("robot-ci", new OAuthInfo(CollectionUtils.linkedSet("d:e", "a:b")));
        whenBlackBoxOAuthThenReturn(response, token);

        assertThatThrownBy(() ->
                pingService.withCallCredentials(new OAuthCallCredentials(token))
                        .ping(Empty.getDefaultInstance())
        )
                .isInstanceOf(StatusRuntimeException.class)
                .satisfies(exception -> {
                    var statusException = ((StatusRuntimeException) exception);
                    assertThat(statusException.getStatus().getCode()).isEqualTo(io.grpc.Status.Code.UNAUTHENTICATED);
                })
                .hasMessageContaining("No required scope (ci:api) in token for user robot-ci. Got scopes: [d:e, a:b]");
    }


    @Test
    public void withoutOptionalBothTickets() throws IOException, InterruptedException {
        setUpAuth(init -> init
                .mandatoryUserTicket(AuthSettings.NOT_REQUIRED)
                .mandatoryServiceTicket(AuthSettings.NOT_REQUIRED));

        pingService.ping(Empty.getDefaultInstance());

        assertThat(authenticatedUser).isNull();
    }

    @Test
    public void testParseIpv6Address() throws UnknownHostException {
        when(tvmClient.checkUserTicket(USER_TICKET))
                .thenReturn(userTicketWith(TicketStatus.OK));

        String login = "USERNAME";

        whenBlackBoxUserInfoThenReturn(login);

        int seed = 91231;
        Random random = new Random(seed);
        byte[] ip6Bytes = new byte[16];
        random.nextBytes(ip6Bytes);

        InetSocketAddress ip6Address = new InetSocketAddress(InetAddress.getByAddress(ip6Bytes), 1332);

        attributesInterceptor.set(Grpc.TRANSPORT_ATTR_REMOTE_ADDR, ip6Address);

        pingService.withCallCredentials(withUserTicket())
                .ping(Empty.getDefaultInstance());
    }

    @Test
    public void testParseIpv6AddressWithScope() throws UnknownHostException {
        when(tvmClient.checkUserTicket(USER_TICKET))
                .thenReturn(userTicketWith(TicketStatus.OK));
        String login = "USERNAME";

        whenBlackBoxUserInfoThenReturn(login);

        int seed = 91231;
        Random random = new Random(seed);
        byte[] ip6Bytes = new byte[16];
        random.nextBytes(ip6Bytes);

        InetSocketAddress ip6Address = new InetSocketAddress(Inet6Address.getByAddress(null, ip6Bytes, 0), 1332);

        attributesInterceptor.set(Grpc.TRANSPORT_ATTR_REMOTE_ADDR, ip6Address);

        pingService.withCallCredentials(withUserTicket())
                .ping(Empty.getDefaultInstance());
    }

    //

    private CheckedUserTicket userTicketWith(TicketStatus status) {
        return Unittest.createUserTicket(status, USER_UID, new String[]{}, new long[]{USER_UID});
    }

    private CheckedServiceTicket serviceTicketWith(TicketStatus status) {
        return serviceTicketWith(status, 0);
    }

    private CheckedServiceTicket serviceTicketWith(TicketStatus status, long issuerId) {
        return Unittest.createServiceTicket(status, SERVICE_ID, issuerId);
    }

    private CallCredentials withUserTicket() {
        return new CallCredentials() {
            @Override
            public void applyRequestMetadata(RequestInfo requestInfo, Executor appExecutor, MetadataApplier applier) {
                Metadata metadata = new Metadata();
                metadata.put(TvmUserTickets.TVM_USER_TICKET_HEADER, USER_TICKET);
                applier.apply(metadata);
            }

            @Override
            public void thisUsesUnstableApi() {
            }
        };
    }

    private CallCredentials withServiceTicket() {
        return new CallCredentials() {
            @Override
            public void applyRequestMetadata(RequestInfo requestInfo, Executor appExecutor, MetadataApplier applier) {
                Metadata metadata = new Metadata();
                metadata.put(TvmServiceTickets.TVM_SERVICE_TICKET_HEADER, SERVICE_TICKET);
                applier.apply(metadata);
            }

            @Override
            public void thisUsesUnstableApi() {
            }
        };
    }


    private CallCredentials withUserAndServiceTicket() {
        return new CallCredentials() {
            @Override
            public void applyRequestMetadata(RequestInfo requestInfo, Executor appExecutor, MetadataApplier applier) {
                Metadata metadata = new Metadata();
                metadata.put(TvmUserTickets.TVM_USER_TICKET_HEADER, USER_TICKET);
                metadata.put(TvmServiceTickets.TVM_SERVICE_TICKET_HEADER, SERVICE_TICKET);
                applier.apply(metadata);
            }

            @Override
            public void thisUsesUnstableApi() {
            }
        };
    }

    private void whenBlackBoxUserInfoThenReturn(String login) {
        var response = new UserInfoResponse(login);
        whenBlackBoxUserInfoThenReturn(response);
    }

    private void whenBlackBoxUserInfoThenReturn(UserInfoResponse response) {
        when(blackboxClient.getUserInfo(anyString(), anyLong()))
                .thenReturn(response);
    }

    private void whenBlackBoxOAuthThenReturn(OAuthResponse response, String token) {
        when(blackboxClient.getOAuth(anyString(), eq(token)))
                .thenReturn(response);
    }

    private void setUpAuth(Consumer<AuthSettings.Builder> init) throws IOException, InterruptedException {
        this.tearDownAuth();

        var tvmSettings = AuthSettings.builder()
                .tvmClient(tvmClient)
                .blackbox(blackboxClient);
        init.accept(tvmSettings);
        yandexAuthInterceptor = new YandexAuthInterceptor(tvmSettings.build());

        String name = InProcessServerBuilder.generateName();
        attributesInterceptor = new CallAttributesInterceptor();
        mockServer = InProcessServerBuilder.forName(name)
                .addService(new InternalApiGrpc.InternalApiImplBase() {
                    @Override
                    public void ping(Empty request, StreamObserver<InternalApiOuterClass.Time> responseObserver) {
                        if (serviceHandler != null) {
                            serviceHandler.ping(request, responseObserver);
                        } else {
                            responseObserver.onNext(InternalApiOuterClass.Time.getDefaultInstance());
                            responseObserver.onCompleted();
                        }
                    }
                })
                .intercept(yandexAuthInterceptor)
                .intercept(attributesInterceptor)
                .build();

        mockServer.start();

        channel = InProcessChannelBuilder.forName(name).directExecutor().build();
        pingService = InternalApiGrpc.newBlockingStub(channel);

        InetSocketAddress fakeRemoteAddr = new InetSocketAddress(InetAddress.getByName(IP_ADDRESS), 1332);
        attributesInterceptor.set(Grpc.TRANSPORT_ATTR_REMOTE_ADDR, fakeRemoteAddr);

        authenticatedUser = null;
        serviceHandler = new InternalApiGrpc.InternalApiImplBase() {
            @Override
            public void ping(Empty request, StreamObserver<InternalApiOuterClass.Time> responseObserver) {
                authenticatedUser = YandexAuthInterceptor.getAuthenticatedUser().orElse(null);
                responseObserver.onNext(InternalApiOuterClass.Time.getDefaultInstance());
                responseObserver.onCompleted();
            }
        };

    }

    private void tearDownAuth() throws InterruptedException {
        assertThat(YandexAuthInterceptor.getAuthenticatedUser())
                .withFailMessage("user should not be accessible out of context")
                .isEmpty();

        if (mockServer != null) {
            mockServer.shutdownNow();
            mockServer.awaitTermination(3, TimeUnit.SECONDS);
            mockServer = null;
        }

        if (channel != null) {
            channel.shutdownNow();
            channel.awaitTermination(3, TimeUnit.SECONDS);
            channel = null;
        }
    }
}


