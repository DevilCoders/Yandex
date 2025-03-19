package yandex.cloud.ti.billing.client;

import lombok.Value;

@Value
public class BillingOperation {

    String id;

    boolean done;

    BillingError error;

    BillingAccount response;

}
