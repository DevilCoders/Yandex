package ru.yandex.ci.core.spring.clients;

import org.mockito.Mockito;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.ci.client.pciexpress.PciExpressClient;

@Configuration
public class PciExpressTestConfig {

    @Bean
    public PciExpressClient pciExpressClient() {
        return Mockito.mock(PciExpressClient.class);
    }

}
