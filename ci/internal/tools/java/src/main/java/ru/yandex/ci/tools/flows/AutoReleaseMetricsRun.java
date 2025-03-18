package ru.yandex.ci.tools.flows;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.engine.launch.auto.AutoReleaseMetrics;
import ru.yandex.ci.engine.spring.DiscoveryConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Configuration
@Import(DiscoveryConfig.class)
public class AutoReleaseMetricsRun extends AbstractSpringBasedApp {

    @Autowired
    AutoReleaseMetrics autoReleaseMetrics;

    @Override
    protected void run() {
        autoReleaseMetrics.run();
    }

    public static void main(String[] args) {
        startAndStopThisClass(args);
    }
}
