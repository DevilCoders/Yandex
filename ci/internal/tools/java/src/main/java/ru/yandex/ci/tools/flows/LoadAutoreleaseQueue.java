package ru.yandex.ci.tools.flows;

import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.engine.launch.auto.AutoReleaseQueue;
import ru.yandex.ci.engine.spring.AutoReleaseConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Configuration
@Import(AutoReleaseConfig.class)
public class LoadAutoreleaseQueue extends AbstractSpringBasedApp {

    @Autowired
    AutoReleaseQueue autoReleaseQueue;

    @Override
    protected void run() {
        log.info("Dump: {}", autoReleaseQueue.getQueueSnapshotDebugString());
    }

    public static void main(String[] args) {
        startAndStopThisClass(args, Environment.STABLE);
    }
}
