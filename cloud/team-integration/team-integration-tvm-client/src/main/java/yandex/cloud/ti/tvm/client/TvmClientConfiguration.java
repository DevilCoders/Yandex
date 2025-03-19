package yandex.cloud.ti.tvm.client;

import org.jetbrains.annotations.Nullable;
import yandex.cloud.auth.api.CloudAuthClient;
import yandex.cloud.grpc.ExceptionMapper;
import yandex.cloud.iam.client.tvm.TvmClient;
import yandex.cloud.iam.client.tvm.config.TvmClientConfig;
import yandex.cloud.iam.client.tvm.grpc.TvmAuthenticationInterceptor;

public class TvmClientConfiguration extends yandex.cloud.iam.client.tvm.staticdi.TvmClientConfiguration {

    @Override
    protected void configure() {
        super.configure();
        put(TvmAuthenticationInterceptor.class, this::createTvmAuthenticationInterceptor);
    }


    protected @Nullable TvmAuthenticationInterceptor createTvmAuthenticationInterceptor() {
        return new TvmAuthenticationInterceptor(
                get(CloudAuthClient.class),
                get(ExceptionMapper.class)::map,
                get(TvmClient.class),
                get(TvmClientConfig.class)
        );
    }

}
