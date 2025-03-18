package ru.yandex.ci.event.spring;

import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.PropertySource;

@Configuration
@PropertySource("classpath:ci-event-reader.properties")
public class CiEventReaderPropertiesConfig {
}
