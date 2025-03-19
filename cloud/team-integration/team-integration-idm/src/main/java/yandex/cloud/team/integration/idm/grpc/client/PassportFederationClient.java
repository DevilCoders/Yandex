package yandex.cloud.team.integration.idm.grpc.client;

import java.util.List;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.grpc.client.AbstractGrpcClient;
import yandex.cloud.grpc.client.RetryStrategy;
import yandex.cloud.team.integration.idm.model.Subject;

public interface PassportFederationClient {

    PassportFederationClient withIdempotencyKey(String newIdempotencyKey);

    PassportFederationClient withRetryStrategy(RetryStrategy retryStrategy);

    PassportFederationClient withToken(AbstractGrpcClient.Token token);

    List<Subject> addUserAccounts(@NotNull long... uids);

    List<Subject> addUserAccounts(@NotNull String... logins);

}
