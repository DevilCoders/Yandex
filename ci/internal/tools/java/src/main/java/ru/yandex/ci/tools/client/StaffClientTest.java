package ru.yandex.ci.tools.client;

import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.staff.StaffClient;
import ru.yandex.ci.engine.spring.clients.StaffClientConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Configuration
@Import(StaffClientConfig.class)
public class StaffClientTest extends AbstractSpringBasedApp {

    @Autowired
    StaffClient staffClient;

    @Override
    protected void run() {
        log.info("{}", staffClient.getStaffPerson("miroslav2"));
        log.info("{}", staffClient.getStaffPerson("robot-ci"));
    }

    public static void main(String[] args) {
        startAndStopThisClass(args, Environment.STABLE);
    }
}
