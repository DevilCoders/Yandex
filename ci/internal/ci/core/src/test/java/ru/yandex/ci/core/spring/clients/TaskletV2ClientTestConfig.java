package ru.yandex.ci.core.spring.clients;

import java.time.Clock;
import java.util.UUID;
import java.util.concurrent.ExecutorService;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.taskletv2.TaskletV2TestServer;
import ru.yandex.ci.common.grpc.GrpcClientProperties;
import ru.yandex.ci.common.grpc.GrpcClientPropertiesStub;
import ru.yandex.ci.core.taskletv2.TaskletV2MetadataHelper;

@Configuration
@Import(TaskletV2ClientConfig.class)
public class TaskletV2ClientTestConfig {

    @Bean
    public String taskletV2Channel() {
        return "taskletV2Channel-" + UUID.randomUUID();
    }

    @Bean
    public TaskletV2TestServer taskletV2TestServer(String taskletV2Channel, Clock clock, ExecutorService testExecutor) {
        return new TaskletV2TestServer(taskletV2Channel, clock, testExecutor);
    }

    @Bean
    public TaskletV2MetadataHelper taskletV2MetadataHelper(TaskletV2TestServer taskletV2TestServer) {
        return new TaskletV2MetadataHelper(taskletV2TestServer);
    }

    @Bean
    public GrpcClientProperties taskletV2ClientProperties(String taskletV2Channel) {
        return GrpcClientPropertiesStub.of(taskletV2Channel);
    }

}
