package yandex.cloud.ti.billing.client;

import java.util.function.Function;

import javax.inject.Inject;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.iam.grpc.remoteoperation.RemoteOperation;
import yandex.cloud.iam.grpc.remoteoperation.RemoteOperationCall;
import yandex.cloud.iam.grpc.remoteoperation.StatusOrOperation;

public class CreateBillingAccountRemoteOpCall extends RemoteOperationCall.BaseCall<BillingAccount> {

    @Inject
    private static BillingPrivateClient billingPrivateClient;

    protected CreateBillingAccountRemoteOpCall(
        Function<String, StatusOrOperation<BillingAccount>> createOperation,
        Function<RemoteOperation.Id, StatusOrOperation<BillingAccount>> getOperation
    ) {
        super(createOperation, getOperation);
    }

    public static CreateBillingAccountRemoteOpCall createCreateBillingAccount(
        @NotNull String name,
        @NotNull String cloudId
    ) {
        return createCreateBillingAccount(billingPrivateClient, name, cloudId);
    }

    public static CreateBillingAccountRemoteOpCall createCreateBillingAccount(
        @NotNull BillingPrivateClient billingPrivateClient,
        @NotNull String name,
        @NotNull String cloudId
    ) {
        return new CreateBillingAccountRemoteOpCall(
            ik -> billingPrivateClient/*.withIdempotencyKey(ik)*/
                .createBillingAccount(name, cloudId),
            getOperation(billingPrivateClient)
        );
    }

    private static Function<RemoteOperation.Id, StatusOrOperation<BillingAccount>> getOperation(
        @NotNull BillingPrivateClient billingPrivateClient
    ) {
        return op -> billingPrivateClient.getBillingAccountOperation(op.getValue());
    }

}
