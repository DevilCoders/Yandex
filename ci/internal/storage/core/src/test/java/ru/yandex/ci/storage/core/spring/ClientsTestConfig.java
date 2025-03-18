package ru.yandex.ci.storage.core.spring;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.ArcServiceStub;
import ru.yandex.ci.core.test.TestData;

@Configuration
@Import({
        ClientsConfig.class
})
public class ClientsTestConfig {

    @Bean
    public ArcService arcService() {
        return new ArcServiceStub(
                "autocheck/postcommits/test-repo",
                TestData.TRUNK_COMMIT_2,
                TestData.TRUNK_COMMIT_3
        );
    }

}
