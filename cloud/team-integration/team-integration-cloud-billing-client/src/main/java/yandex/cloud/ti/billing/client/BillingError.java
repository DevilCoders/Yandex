package yandex.cloud.ti.billing.client;

import lombok.Value;

@Value
public class BillingError {

    int code;

    String message;

    Details[] details;

    @Value
    public static class Details {

        int code;

        String message;

        String type;

    }

}
