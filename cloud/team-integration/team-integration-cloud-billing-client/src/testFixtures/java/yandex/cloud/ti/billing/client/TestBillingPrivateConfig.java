package yandex.cloud.ti.billing.client;

import yandex.cloud.common.httpserver.HttpServer;
import yandex.cloud.common.httpserver.config.LocalHttpClientConfig;

public class TestBillingPrivateConfig extends LocalHttpClientConfig implements BillingPrivateConfig {

    public TestBillingPrivateConfig(HttpServer localServer) {
        super(localServer);
    }

}
