package ru.yandex.ci.tools.client;

import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.yav.YavClient;
import ru.yandex.ci.engine.spring.clients.SecurityClientsConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Import(SecurityClientsConfig.class)
@Configuration
public class YavClientTest extends AbstractSpringBasedApp {

    @Autowired
    YavClient yavClient;

    @Override
    protected void run() {
        log.info(yavClient.getReaders("sec-01e8agdtdcs61v6emr05h5q1ek"));
        log.info(yavClient.getReaders("sec-01ff21rj56q4r6c2ff5tbjm6k8"));
    }

    public static void main(String[] args) {
        startAndStopThisClass(args, Environment.STABLE);
    }

}
