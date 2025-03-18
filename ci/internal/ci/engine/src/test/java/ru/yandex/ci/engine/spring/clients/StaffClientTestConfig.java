package ru.yandex.ci.engine.spring.clients;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.ci.client.staff.StaffClient;

import static org.mockito.Mockito.mock;

@Configuration
public class StaffClientTestConfig {

    @Bean
    public StaffClient staffClient() {
        return mock(StaffClient.class);
    }

}
