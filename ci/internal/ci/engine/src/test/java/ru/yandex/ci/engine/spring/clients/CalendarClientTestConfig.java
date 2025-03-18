package ru.yandex.ci.engine.spring.clients;

import org.mockito.Mockito;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.ci.client.calendar.CalendarClient;

@Configuration
public class CalendarClientTestConfig {

    @Bean
    public CalendarClient calendarClient() {
        return Mockito.mock(CalendarClient.class);
    }

}
