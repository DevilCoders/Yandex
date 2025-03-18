package ru.yandex.ci.tools.yamls;

import lombok.extern.slf4j.Slf4j;
import org.springframework.context.annotation.Configuration;

import ru.yandex.ci.core.db.model.ConfigState;
import ru.yandex.ci.tms.metric.ci.CiSelfUsageMetricCronTask;
import ru.yandex.ci.tools.ProcessAYamlsBase;

@Slf4j
@Configuration
public class CollectUsageMetrics extends ProcessAYamlsBase {

    @Override
    protected void run() throws Exception {
        var counter = new CiSelfUsageMetricCronTask.Counter();
        var scanner = new YamlScanner() {
            @Override
            protected boolean acceptConfig(ConfigState cfg) {
                log.info("Config: {}", cfg.getConfigPath());
                counter.collect(cfg);
                return false;
            }
        };
        scanner.checkAllConfigs(path -> {
        });
        log.info("Counter: {}", counter);
    }

    public static void main(String[] args) {
        startAndStopThisClass(args, Environment.STABLE);
    }
}
