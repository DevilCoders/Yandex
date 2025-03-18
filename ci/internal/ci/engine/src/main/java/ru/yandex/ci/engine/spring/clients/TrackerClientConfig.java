package ru.yandex.ci.engine.spring.clients;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.ci.client.tracker.TrackerClient;

@Configuration
public class TrackerClientConfig {

    @Bean
    public TrackerClient trackerClient(@Value("${ci.trackerClient.url}") String url) {
        return TrackerClient.create(url);
    }
}
