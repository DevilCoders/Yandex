package yandex.cloud.team.integration.idm.grpc.client;

import java.time.Duration;
import java.util.Arrays;
import java.util.List;
import java.util.stream.Collectors;

import com.google.protobuf.InvalidProtocolBufferException;
import io.grpc.ManagedChannel;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.converter.ConverterBuilder;
import yandex.cloud.grpc.client.AbstractGrpcClient;
import yandex.cloud.grpc.client.ClientConfig;
import yandex.cloud.grpc.client.InProcessClientConfig;
import yandex.cloud.grpc.client.RetryStrategy;
import yandex.cloud.grpc.client.SyncCallRetryStrategy;
import yandex.cloud.priv.iam.v1.PUA;
import yandex.cloud.priv.iam.v1.transitional.PTPFS;
import yandex.cloud.priv.iam.v1.transitional.PassportFederationServiceGrpc;
import yandex.cloud.team.integration.idm.model.Subject;
import yandex.cloud.team.integration.idm.model.SubjectType;

public class PassportFederationClientImpl extends AbstractGrpcClient<PassportFederationClientImpl> implements PassportFederationClient {

    private static final String CLIENT_NAME = "passport-federation";

    private final PassportFederationServiceGrpc.PassportFederationServiceBlockingStub passportFederation;

    public PassportFederationClientImpl(String applicationName, ClientConfig config, Token token) {
        this(applicationName, buildChannel(config), config.getTimeout(), token, null, SyncCallRetryStrategy.INSTANCE);
    }

    public PassportFederationClientImpl(String applicationName, InProcessClientConfig config, Token token) {
        this(applicationName, buildChannel(config), null, token, null, SyncCallRetryStrategy.INSTANCE);
    }

    private PassportFederationClientImpl(String applicationName, ManagedChannel channel, Duration timeout, Token token,
                                         String idempotencyKey, RetryStrategy retryStrategy) {
        super(applicationName, channel, timeout, CLIENT_NAME, token, idempotencyKey, retryStrategy);

        this.passportFederation = intercept(PassportFederationServiceGrpc.newBlockingStub(channel));
    }

    @Override
    public PassportFederationClientImpl withIdempotencyKey(String newIdempotencyKey) {
        return new PassportFederationClientImpl(applicationName, channel, timeout, token, newIdempotencyKey, retryStrategy);
    }

    @Override
    public PassportFederationClientImpl withRetryStrategy(RetryStrategy newRetryStrategy) {
        return new PassportFederationClientImpl(applicationName, channel, timeout, token, idempotencyKey, newRetryStrategy);
    }

    @Override
    public PassportFederationClientImpl withToken(Token newToken) {
        return new PassportFederationClientImpl(applicationName, channel, timeout, newToken, idempotencyKey, retryStrategy);
    }

    @Override
    public List<Subject> addUserAccounts(@NotNull long... uids) {
        return addUserAccounts(createAddPassportUserAccountsRequest(uids));
    }

    @Override
    public List<Subject> addUserAccounts(@NotNull String... logins) {
        return addUserAccounts(createAddPassportUserAccountsRequest(logins));
    }

    private List<Subject> addUserAccounts(@NotNull PTPFS.AddPassportUserAccountsRequest request) {
        var response = safeGet(() -> passportFederation.addUserAccounts(request))
                .getResponse();
        PTPFS.AddUserAccountsResponse userAccounts = null;
        try {
            userAccounts = response.unpack(PTPFS.AddUserAccountsResponse.class);
        } catch (InvalidProtocolBufferException e) {
            throw new RuntimeException(e);
        }
        return toSubjects(userAccounts.getUserAccountsList());
    }

    private static List<Subject> toSubjects(List<PUA.UserAccount> userAccounts) {
        return userAccounts.stream()
            .map(userAccount -> Subject.of(SubjectType.USER_ACCOUNT, userAccount.getId()))
            .collect(Collectors.toUnmodifiableList());
    }

    public static PTPFS.AddPassportUserAccountsRequest createAddPassportUserAccountsRequest(@NotNull long... uids) {
        return ConverterBuilder.convert(PTPFS.AddPassportUserAccountsRequest.newBuilder())
            .set(PTPFS.AddPassportUserAccountsRequest.Builder::setUids,
                ConverterBuilder.convert(PTPFS.Uids.newBuilder()
                    .addAllValues(Arrays.stream(uids).boxed().collect(Collectors.toList())))
                    .getDst())
            .getDst()
            .build();
    }

    public static PTPFS.AddPassportUserAccountsRequest createAddPassportUserAccountsRequest(@NotNull String... logins) {
        return ConverterBuilder.convert(PTPFS.AddPassportUserAccountsRequest.newBuilder())
            .set(PTPFS.AddPassportUserAccountsRequest.Builder::setLogins,
                ConverterBuilder.convert(PTPFS.Logins.newBuilder()
                    .addAllValues(List.of(logins)))
                    .getDst())
            .getDst()
            .build();
    }

}
