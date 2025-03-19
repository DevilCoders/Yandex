package yandex.cloud.ti.billing.client;

import java.net.http.HttpRequest;
import java.util.function.Function;
import java.util.function.Supplier;

import io.grpc.Status;
import lombok.extern.log4j.Log4j2;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.grpc.client.ClientConfig;
import yandex.cloud.http.Headers;
import yandex.cloud.iam.grpc.remoteoperation.RemoteOperation;
import yandex.cloud.iam.grpc.remoteoperation.StatusOrOperation;
import yandex.cloud.ti.http.client.AbstractHttpClient;
import yandex.cloud.ti.http.client.HttpClientException;

/**
 * Client for Billing private API.
 */
@Log4j2
public class BillingPrivateClientImpl extends AbstractHttpClient implements BillingPrivateClient {

    private static final String API_PREFIX = "/billing/v1/private/";

    public BillingPrivateClientImpl(
        @NotNull String userAgent,
        @NotNull ClientConfig endpoint,
        @NotNull Supplier<String> token
    ) {
        this(userAgent, endpoint, token, null);
    }

    public BillingPrivateClientImpl(
        @NotNull String userAgent,
        @NotNull ClientConfig endpoint,
        @NotNull Supplier<String> token,
        String idempotencyKey
    ) {
        super(endpoint, API_PREFIX, userAgent, token, idempotencyKey);
    }

    @Override
    public @NotNull BillingPrivateClient withIdempotencyKey(@NotNull String idempotencyKey) {
        return new BillingPrivateClientImpl(userAgent, endpoint, token, idempotencyKey);
    }

    @Override
    public @NotNull StatusOrOperation<BillingAccount> createBillingAccount(
        @NotNull String name,
        @NotNull String cloudId
    ) {
        var path = "billingAccounts/createInternal";
        var request = CreateBillingAccountRequest.of(name,
            CreateBillingAccountRequest.PAYMENT_TYPE_CARD, cloudId);

        return sendOperationRequest(path, null, request, this::convertOperation, BillingOperation.class);
    }

    @Override
    public @NotNull StatusOrOperation<BillingAccount> getBillingAccountOperation(
            @NotNull String operationId
    ) {
        var path = "operations/" + operationId;

        return sendOperationRequest(path, null, null, this::convertOperation, BillingOperation.class);
    }

    private <RESULT, DATA, OPERATION> StatusOrOperation<RESULT> sendOperationRequest(
            String path,
            String query,
            DATA data,
            @NotNull Function<OPERATION, RemoteOperation<RESULT>> operationMapper,
            @NotNull Class<OPERATION> operationType
    ) {
        try {
            var operation = sendRequest(path, query, data, operationType);
            return StatusOrOperation.result(operationMapper.apply(operation));
        } catch (HttpClientException ex) {
            return StatusOrOperation.status(Status.UNAVAILABLE.withCause(ex).asRuntimeException());
        }
    }

    private @NotNull RemoteOperation<BillingAccount> convertOperation(
            @NotNull BillingOperation operation
    ) {
        var id = RemoteOperation.Id.of(operation.getId());

        return operation.isDone()
                ? operation.getError() != null
                ? RemoteOperation.failure(id, com.google.rpc.Status.newBuilder()
                .setCode(mapBillingError(operation.getError().getCode()))
                    .setMessage(operation.getError().getMessage())
                    .build())
                : RemoteOperation.success(id, operation.getResponse())
            : RemoteOperation.running(id);
    }

    private int mapBillingError(int code) {
        if (code == Status.Code.ALREADY_EXISTS.value()) {
            return code;
        }
        return Status.Code.UNAVAILABLE.value();
    }

    @Override
    protected HttpRequest.Builder prepareRequest(
        @NotNull HttpRequest.Builder builder
    ) {
        return builder
            .setHeader(Headers.YA_CLOUD_TOKEN, token.get());
    }

}
