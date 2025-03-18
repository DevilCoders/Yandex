package ru.yandex.ci.engine.spring;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.engine.autocheck.AutocheckBlacklistService;
import ru.yandex.ci.engine.autocheck.AutocheckBootstrapServicePostCommits;
import ru.yandex.ci.engine.autocheck.AutocheckBootstrapServicePullRequests;
import ru.yandex.ci.engine.config.BranchYamlService;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.engine.launch.LaunchService;
import ru.yandex.ci.engine.pr.PullRequestService;

@Configuration
@Import({
        AutocheckConfig.class,
        LaunchConfig.class,
})
public class AutocheckBootstrapConfig {

    @Bean
    public AutocheckBootstrapServicePullRequests autocheckBootstrapServicePullRequest(
            ConfigurationService configurationService,
            LaunchService launchService,
            PullRequestService pullRequestService,
            BranchYamlService branchYamlService,
            CiMainDb db,
            AutocheckBlacklistService autocheckBlacklistService,
            ArcService arcService) {
        return new AutocheckBootstrapServicePullRequests(
                configurationService,
                launchService,
                pullRequestService,
                branchYamlService,
                db,
                autocheckBlacklistService,
                arcService
        );
    }

    @Bean
    public AutocheckBootstrapServicePostCommits autocheckBootstrapServicePostCommits(
            ConfigurationService configurationService,
            LaunchService launchService,
            CiMainDb db
    ) {
        return new AutocheckBootstrapServicePostCommits(configurationService, launchService, db);
    }

}
