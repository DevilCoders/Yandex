package ru.yandex.ci.tools.client;

import lombok.extern.slf4j.Slf4j;
import org.springframework.context.annotation.Configuration;

import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.oldci.OldCiClient;
import ru.yandex.ci.tools.AbstractSpringBasedApp;
import ru.yandex.ci.tools.Misc;

@Slf4j
@Configuration
public class CiClientTest extends AbstractSpringBasedApp {

    @Override
    protected void run() {
        var ciClient = OldCiClient.create(HttpClientProperties.ofEndpoint(Misc.OLD_CI_API_URL));
        var check = ciClient.getCheck("4vqjn");
        log.info("Check: {}", check);
    }

    public static void main(String[] args) {
        startAndStopThisClass(args);
    }
}
