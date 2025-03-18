package ru.yandex.ci.tools;

import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.PropertySource;

// TODO: make better
@Configuration
@PropertySource("classpath:ci-core/ci-common-stable.properties")
@PropertySource(
        value = "classpath:ci-storage-core/ci-storage-stable.properties",
        ignoreResourceNotFound = true
)
@PropertySource(
        value = "classpath:ci-observer-core/ci-observer-stable.properties",
        ignoreResourceNotFound = true
)
public class PropertySourceStable {

}
