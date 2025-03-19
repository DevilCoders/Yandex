package yandex.cloud.team.integration.application;

import yandex.cloud.application.ServiceApplication;
import yandex.cloud.di.StaticDI;
import yandex.cloud.team.integration.config.ProductionConfiguration;

public class Application {

    /**
     * The application name. Used for logs and metrics.
     */
    public static final String NAME = "team-integration";

    /**
     * The prefix to limit dependency injection.
     */
    public static final String PACKAGE = "yandex.cloud.team.integration";

    /**
     * Prevent any instances.
     */
    private Application() {
    }

    /**
     * The entry point.
     *
     * @param arguments the command line arguments
     */
    public static void main(String[] arguments) {
        ServiceApplication.run(NAME, () -> {
            StaticDI.inject(new ProductionConfiguration()).to(
                    PACKAGE,                     // this application
                    "yandex.cloud.ti",
                    "yandex.cloud.audit",        // OperationContext2AwareJob
                    "yandex.cloud.client",       // AbstractGrpcCall, TokenCall
                    "yandex.cloud.iam.service",  // OperationCallHandler, TransactionHandler
                    "yandex.cloud.metrics",      // PrometheusMetricReporter
                    "yandex.cloud.repository",   // IdGenerator
                    "yandex.cloud.iam.grpc.remoteoperation" // RemoteOperationProcess
            );
            return new Lifecycle();
        });
    }

}
