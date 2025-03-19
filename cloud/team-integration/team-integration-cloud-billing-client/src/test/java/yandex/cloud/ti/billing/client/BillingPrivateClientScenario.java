package yandex.cloud.ti.billing.client;

import javax.inject.Inject;

import io.grpc.Status;
import org.assertj.core.api.Assertions;
import org.junit.Test;
import yandex.cloud.scenario.AbstractScenario;
import yandex.cloud.scenario.DependsOn;

@DependsOn(BillingPrivateClientScenarioSuite.class)
public class BillingPrivateClientScenario extends AbstractScenario {

    @Inject
    private static BillingPrivateClient billingPrivateClient;

    @Override
    public void main() {
        var operation = billingPrivateClient
                .createBillingAccount("name", "id");
        Assertions.assertThat(operation)
                .isNotNull()
                .extracting(statusOrOperation -> statusOrOperation.getOperation().isDone())
                .isEqualTo(false);

        operation = billingPrivateClient
                .getBillingAccountOperation(operation.getOperation().getId().getValue());
        Assertions.assertThat(operation)
                .extracting(statusOrOperation -> statusOrOperation.getOperation().isDone())
                .isEqualTo(true);
    }

    @Test
    public void testDeserializeFailedOperation() {
        var operation = billingPrivateClient
                .getBillingAccountOperation("op-error-response");

        Assertions.assertThat(operation.getResult().getStatusOrResult().getStatusCode())
                .isEqualTo(Status.Code.UNAVAILABLE);
    }

    @Test
    public void testAlreadyBoundCloudResponse() {
        var operation = billingPrivateClient
                .getBillingAccountOperation("op-ba-already-bound");

        Assertions.assertThat(operation.getResult().getStatusOrResult().getStatusCode())
                .isEqualTo(Status.Code.ALREADY_EXISTS);
    }

}
