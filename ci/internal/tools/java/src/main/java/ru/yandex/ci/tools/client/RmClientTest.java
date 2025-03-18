package ru.yandex.ci.tools.client;

import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.rm.RmClient;
import ru.yandex.ci.tms.spring.clients.RmClientConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Import(RmClientConfig.class)
@Configuration
public class RmClientTest extends AbstractSpringBasedApp {

    @Autowired
    RmClient rmClient;

    @Override
    protected void run() {
        log.info("Components: {}", rmClient.getComponents());
    }

    public static void main(String[] args) {
        startAndStopThisClass(args, Environment.STABLE);
    }
}
