package yandex.cloud.ti.billing.client;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.iam.grpc.remoteoperation.StatusOrOperation;

public interface BillingPrivateClient {

    @NotNull BillingPrivateClient withIdempotencyKey(
        @NotNull String idempotencyKey
    );

    @NotNull StatusOrOperation<BillingAccount> createBillingAccount(
        @NotNull String name,
        @NotNull String cloudId
    );

    @NotNull StatusOrOperation<BillingAccount> getBillingAccountOperation(
        @NotNull String operationId
    );

}
