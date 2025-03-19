package yandex.cloud.team.integration.abc;

import io.grpc.StatusRuntimeException;
import org.assertj.core.api.Assertions;
import org.junit.Test;
import yandex.cloud.grpc.GrpcHeaders;
import yandex.cloud.grpc.HeaderAttachingClientInterceptor;
import yandex.cloud.priv.team.integration.v1.PTIAS;
import yandex.cloud.scenario.DependsOn;

@DependsOn(AbcIntegrationScenarioSuite.class)
public class AuthScenario extends AbcIntegrationScenarioBase {

    @Override
    @Test
    public void main() {
        Assertions.assertThatExceptionOfType(StatusRuntimeException.class)
                .isThrownBy(() -> getAbcServiceClient().getAbcService()
                        .withInterceptors(
                                new HeaderAttachingClientInterceptor(GrpcHeaders.AUTHORIZATION, "Bearer ...")
                        )
                        .create(PTIAS.CreateCloudRequest.getDefaultInstance()))
                .withMessage("INVALID_ARGUMENT: Validation failed:\n" +
                        "  - abc: One of the options must be selected");

        Assertions.assertThatExceptionOfType(StatusRuntimeException.class)
                .isThrownBy(() -> getAbcServiceClient().getAbcService()
                        .withInterceptors(
                                new HeaderAttachingClientInterceptor(GrpcHeaders.AUTHORIZATION, "Bearer ...")
                        )
                        .resolve(PTIAS.ResolveRequest.getDefaultInstance()))
                .withMessage("INVALID_ARGUMENT: Validation failed:\n" +
                        "  - abc: One of the options must be selected");
    }

    @Test
    public void testTvmAuth() {
        Assertions.assertThatExceptionOfType(StatusRuntimeException.class)
                .isThrownBy(() -> getAbcServiceClient().getAbcService()
                        .withInterceptors(
                                new HeaderAttachingClientInterceptor(GrpcHeaders.X_YA_SERVICE_TICKET, "TVM:0:0")
                        )
                        .create(PTIAS.CreateCloudRequest.getDefaultInstance()))
                .withMessage("INVALID_ARGUMENT: Validation failed:\n" +
                        "  - abc: One of the options must be selected");
    }

    @Test
    public void testNoAuthRequest() {
        Assertions.assertThatExceptionOfType(StatusRuntimeException.class)
                .isThrownBy(() -> getAbcServiceClient().getAbcService()
                        .create(PTIAS.CreateCloudRequest.getDefaultInstance()))
                .withMessage("UNAUTHENTICATED: Authorization header is missing");
    }

}
