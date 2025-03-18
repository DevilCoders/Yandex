package ru.yandex.ci.tools.flows;

import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.spring.ydb.YdbCiConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Configuration
@Import(YdbCiConfig.class)
public class LoadFlow extends AbstractSpringBasedApp {

    @Autowired
    CiDb db;

    @Override
    protected void run() {
        var id = FlowLaunchId.of("116aac8b6e89616ea28e0716f67856bb15d8d8f9a57cfc031b25a762cdc446ba");
        var flowLaunch = db.currentOrReadOnly(() -> db.flowLaunch().get(id));
        log.info("Flow Launch: {}", flowLaunch);

        var launch = db.currentOrReadOnly(() -> db.launches().get(flowLaunch.getLaunchId()));
        log.info("Launch: {}", launch);
    }

    public static void main(String[] args) {
        startAndStopThisClass(args, Environment.STABLE);
    }
}
