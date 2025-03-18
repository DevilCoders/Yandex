package ru.yandex.ci.common.bazinga.spring;

import com.amazonaws.services.s3.AmazonS3;
import org.mockito.Mockito;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

@Configuration
public class S3LogStorageTestConfig {

    @Bean
    public AmazonS3 amazonS3() {
        return Mockito.mock(AmazonS3.class);
    }
}
