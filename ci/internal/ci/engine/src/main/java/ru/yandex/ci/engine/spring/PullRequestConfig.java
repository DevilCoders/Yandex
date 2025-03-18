package ru.yandex.ci.engine.spring;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.arcanum.ArcanumClientImpl;
import ru.yandex.ci.common.bazinga.spring.BazingaCoreConfig;
import ru.yandex.ci.core.security.PermissionsService;
import ru.yandex.ci.engine.pr.PullRequestCommentService;
import ru.yandex.ci.engine.pr.PullRequestService;
import ru.yandex.ci.engine.spring.clients.ArcanumClientConfig;
import ru.yandex.ci.flow.spring.UrlServiceConfig;
import ru.yandex.ci.flow.utils.UrlService;
import ru.yandex.commune.bazinga.BazingaTaskManager;

@Configuration
@Import({
        BazingaCoreConfig.class,
        ArcanumClientConfig.class,
        UrlServiceConfig.class,
        PermissionsConfig.class
})
public class PullRequestConfig {

    @Bean
    public PullRequestCommentService pullRequestCommentService(BazingaTaskManager taskManager) {
        return new PullRequestCommentService(taskManager);
    }

    @Bean
    public PullRequestService pullRequestService(
            ArcanumClientImpl arcanumClient,
            @Value("${ci.pullRequestService.mergeRequirementSystem}") String mergeRequirementSystem,
            UrlService urlService,
            PullRequestCommentService pullRequestCommentService,
            PermissionsService permissionsService
    ) {
        return new PullRequestService(
                arcanumClient,
                mergeRequirementSystem,
                urlService,
                pullRequestCommentService,
                permissionsService
        );
    }
}
