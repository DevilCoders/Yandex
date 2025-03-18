package ru.yandex.ci.api.spring;

import io.grpc.ServerBuilder;
import io.grpc.inprocess.InProcessServerBuilder;
import org.mockito.Mockito;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.ci.client.blackbox.BlackboxClient;
import ru.yandex.passport.tvmauth.TvmClient;

@Configuration
public class CiApiGrpcTestConfig {

    @Bean
    public TvmClient tvmClient() {
        return Mockito.mock(TvmClient.class);
    }

    @Bean
    public BlackboxClient blackbox() {
        return Mockito.mock(BlackboxClient.class);
    }

    @Bean
    public String serverName() {
        return InProcessServerBuilder.generateName();
    }

    @Bean
    public ServerBuilder<?> serverBuilder(String serverName) {
        return InProcessServerBuilder.forName(serverName);
    }
}
