package yandex.cloud.team.integration;

import javax.inject.Inject;

import io.grpc.Server;
import org.assertj.core.api.Assertions;
import org.junit.Test;
import yandex.cloud.di.StaticDI;
import yandex.cloud.team.integration.application.Application;

public class DebugConfigurationTest {

    @Test
    public void loadDebugConfiguration() {
        // todo use the same "to" list here and in the DebugApplication
        StaticDI.inject(new DebugConfiguration()).to(
                Application.PACKAGE,        // this application
                "yandex.cloud.ti",
                "yandex.cloud.audit",       // OperationContext2AwareJob
                "yandex.cloud.client",      // AbstractGrpcCall, TokenCall
                "yandex.cloud.iam.service", // OperationCallHandler, TransactionHandler
                "yandex.cloud.metrics",     // PrometheusMetricReporter
                "yandex.cloud.repository",  // IdGenerator
                "yandex.cloud.iam.grpc.remoteoperation" // RemoteOperationProcess
        );
        Assertions.assertThat(Target.server).isNotNull();
    }

    private static class Target {

        @Inject
        public static Server server;

    }

}
