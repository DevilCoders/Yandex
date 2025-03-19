package yandex.cloud.ti.billing.client;

import lombok.Value;
import yandex.cloud.model.operation.OperationResponse;

@Value
public class BillingAccount implements OperationResponse {

    String id;

}
