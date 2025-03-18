package ru.yandex.ci.tms.spring;


import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.PropertySource;

import ru.yandex.ci.tms.test.internal.InternalJob;

@Configuration
@Import(CiTmsPropertiesTestConfig.class)
@PropertySource("classpath:ci-tms-entire-test.properties")
public class EntireTestConfig {

    @Bean
    public InternalJob.Parameters internalJobParameters() {
        return new InternalJob.Parameters();
    }

    @Bean
    public InternalJob internalJob(InternalJob.Parameters internalJobParameters) {
        return new InternalJob(internalJobParameters);
    }

}
