package ru.yandex.ci.tools.client;

import java.util.List;

import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.abc.AbcClient;
import ru.yandex.ci.client.abc.AbcTableClient;
import ru.yandex.ci.core.spring.clients.AbcClientConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Import(AbcClientConfig.class)
@Configuration
public class AbcClientTest extends AbstractSpringBasedApp {

    @Autowired
    AbcClient abcClient;

    @Autowired
    AbcTableClient abcTableClient;

    @Override
    protected void run() throws Exception {
        abcTableClient();
    }

    void abcClient() {
        log.info("getFirst1000ServicesWhichUserBelongsTo: {}",
                abcClient.getServiceMembers("miroslav2"));
        log.info("getServicesMembers(ci,development,service.slug,person.login, uniq): {}",
                abcClient.getServicesMembers("ci", "development", List.of("service.slug", "person.login"), true));
        log.info("getServiceMembersWithDescendants: {}",
                abcClient.getServiceMembersWithDescendants("ci", List.of("developer")));
        log.info("getServices: {}",
                abcClient.getServices(List.of("ci")));

        log.info("Total services in ABC: {}", abcClient.getAllServices().size());
    }

    void abcTableClient() {
        abcTableClient.getUserToServicesMapping();
    }

    public static void main(String[] args) {
        startAndStopThisClass(args);
    }

}
