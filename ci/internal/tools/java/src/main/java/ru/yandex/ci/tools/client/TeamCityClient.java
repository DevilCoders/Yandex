package ru.yandex.ci.tools.client;

import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.teamcity.TeamcityClient;
import ru.yandex.ci.tms.spring.clients.TeamcityClientConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Import(TeamcityClientConfig.class)
@Configuration
public class TeamCityClient extends AbstractSpringBasedApp {

    @Autowired
    TeamcityClient tcClient;

    @Override
    protected void run() {
        log.info("getArcRoots: {}", tcClient.getArcRoots());
        log.info("getArcadiaSvnRoots: {}", tcClient.getArcadiaSvnRoots());
        log.info("getBuildTypesWithArcRoots: {}", tcClient.getBuildTypesWithArcRoots());
        log.info("getBuildTypesWithArcadiaSvnRoots: {}", tcClient.getBuildTypesWithArcadiaSvnRoots());
    }

    public static void main(String[] args) {
        startAndStopThisClass(args);
    }
}
