package yandex.cloud.team.integration;

import yandex.cloud.application.ServiceApplication;
import yandex.cloud.di.StaticDI;
import yandex.cloud.team.integration.application.Application;
import yandex.cloud.team.integration.application.Lifecycle;

/**
 * Same as {@code Application}, but omits TVM validation and uses fake service account.
 */
public class DebugApplication {

    public static void main(String[] arguments) {
        ServiceApplication.run(Application.NAME, () -> {
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
            return new Lifecycle();
        });
    }

}
