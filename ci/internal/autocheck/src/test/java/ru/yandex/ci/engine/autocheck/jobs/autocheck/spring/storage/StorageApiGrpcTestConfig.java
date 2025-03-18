package ru.yandex.ci.engine.autocheck.jobs.autocheck.spring.storage;

import io.grpc.ServerBuilder;
import io.grpc.inprocess.InProcessServerBuilder;
import org.mockito.Mockito;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.blackbox.BlackboxClient;
import ru.yandex.ci.client.tvm.grpc.AuthSettings;
import ru.yandex.ci.client.tvm.grpc.YandexAuthInterceptor;
import ru.yandex.ci.storage.api.spring.StorageApiGrpcConfig;
import ru.yandex.ci.storage.core.spring.ClientsTestConfig;
import ru.yandex.passport.tvmauth.TvmClient;

@Configuration
@Import({
        ClientsTestConfig.class,
        StorageApiGrpcConfig.class
})
public class StorageApiGrpcTestConfig {

    @Bean
    public TvmClient tvmClient() {
        return Mockito.mock(TvmClient.class);
    }

    @Bean
    public YandexAuthInterceptor tvmAuthInterceptor() {
        return new YandexAuthInterceptor(AuthSettings.builder()
                .tvmClient(Mockito.mock(TvmClient.class))
                .blackbox(Mockito.mock(BlackboxClient.class))
                .debug(true)
                .build());
    }

    @Bean
    public String storageServerName() {
        return InProcessServerBuilder.generateName();
    }

    @Bean
    public ServerBuilder<?> storageServerBuilder(
            @Qualifier("storageServerName") String storageServerName
    ) {
        return InProcessServerBuilder.forName(storageServerName).directExecutor();
    }

}
