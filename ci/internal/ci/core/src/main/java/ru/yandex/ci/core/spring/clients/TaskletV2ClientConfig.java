package ru.yandex.ci.core.spring.clients;

import java.time.Duration;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.client.taskletv2.TaskletV2Client;
import ru.yandex.ci.client.taskletv2.TaskletV2ClientImpl;
import ru.yandex.ci.client.tvm.grpc.OAuthCallCredentials;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.common.grpc.GrpcClientProperties;
import ru.yandex.ci.core.tasklet.TaskletMetadataValidationException;

@Configuration
public class TaskletV2ClientConfig {

    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    @Bean
    public GrpcClientProperties taskletV2ClientProperties(
            @Value("${ci.taskletV2Client.oauthToken}") String oauthToken,
            @Value("${ci.taskletV2ClientProperties.endpoint}") String endpoint,
            @Value("${ci.taskletV2ClientProperties.connectTimeout}") Duration connectTimeout,
            @Value("${ci.taskletV2ClientProperties.deadlineAfter}") Duration deadlineAfter
    ) {
        return GrpcClientProperties.builder()
                .connectTimeout(connectTimeout)
                .deadlineAfter(deadlineAfter)
                .endpoint(endpoint)
                .callCredentials(new OAuthCallCredentials(oauthToken, true))
                .build();
    }

    @Bean
    public TaskletV2Client taskletV2Client(GrpcClientProperties taskletV2ClientProperties) {
        return TaskletV2ClientImpl.create(taskletV2ClientProperties, TaskletMetadataValidationException::new);
    }
}
