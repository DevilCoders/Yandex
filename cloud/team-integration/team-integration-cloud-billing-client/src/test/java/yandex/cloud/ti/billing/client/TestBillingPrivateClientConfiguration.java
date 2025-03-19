package yandex.cloud.ti.billing.client;

import java.util.List;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.auth.SystemAccountService;
import yandex.cloud.common.httpserver.HttpServer;
import yandex.cloud.di.Configuration;
import yandex.cloud.fake.iam.service.FakeSystemAccountService;
import yandex.cloud.ti.billing.FakeBillingPrivateServlet;
import yandex.cloud.ti.http.server.HttpServerConfiguration;
import yandex.cloud.ti.http.server.TestHttpServerConfiguration;

public class TestBillingPrivateClientConfiguration extends Configuration {

    @Override
    protected void configure() {
        merge(new BillingPrivateClientConfiguration(TestBillingPrivateClientConfiguration.class.getSimpleName()));
        put(BillingPrivateConfig.class, () -> new TestBillingPrivateConfig(get(HttpServer.class)));
        merge(httpServerConfiguration());
        put(SystemAccountService.class, FakeSystemAccountService::new);
    }

    protected @NotNull HttpServerConfiguration httpServerConfiguration() {
        return new TestHttpServerConfiguration(
                List.of(),
                List.of(
                        FakeBillingPrivateServlet.class
                )
        );
    }

}
