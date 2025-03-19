package yandex.cloud.ti.billing.client;

import lombok.Value;
import org.jetbrains.annotations.NotNull;

@Value(staticConstructor = "of")
public class CreateBillingAccountRequest {

    public static final String PAYMENT_TYPE_CARD = "card";

    @NotNull String name;

    @NotNull String paymentType;

    @NotNull String cloudId;

}
