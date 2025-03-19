package yandex.cloud.team.integration.idm.core;

import io.grpc.ServerBuilder;
import io.grpc.inprocess.InProcessServerBuilder;
import lombok.Getter;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.fake.iam.FakeIamServer;
import yandex.cloud.grpc.FakeServerInterceptors;
import yandex.cloud.grpc.FakeServerInterceptors.InterceptorConfig;

@Getter
public class IdmServiceTestContextImpl implements IdmServiceTestContext {

    private static final InterceptorConfig FAKE_INTERCEPTORS_CONFIG = InterceptorConfig.DEFAULT;

    private static final String iamChannelName = FakeIamServer.class.getSimpleName();

    private final FakeIamServer iamServer =
            new FakeIamServer(server("fake_iam_inproc", iamChannelName), "fia", "frm");

    private final IdmServiceTestContext.TestInit init = new IdmServiceTestContext.TestInit(iamServer);

    private static ServerBuilder<?> server(@NotNull String serviceName, @NotNull String channelName) {
        return FakeServerInterceptors.intercept(serviceName, InProcessServerBuilder.forName(channelName), FAKE_INTERCEPTORS_CONFIG);
    }

}
