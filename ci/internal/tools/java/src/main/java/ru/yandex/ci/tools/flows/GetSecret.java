package ru.yandex.ci.tools.flows;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.security.YavToken;
import ru.yandex.ci.engine.flow.SecurityAccessService;
import ru.yandex.ci.engine.spring.SecurityServiceConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Configuration
@Import(SecurityServiceConfig.class)
public class GetSecret extends AbstractSpringBasedApp {

    @Autowired
    SecurityAccessService securityAccessService;

    @Override
    protected void run() {
        securityAccessService.getYavSecret(YavToken.Id.of(System.getenv("TID_TOKEN")));
    }

    public static void main(String[] args) {
        startAndStopThisClass(args, Environment.STABLE);
    }
}
