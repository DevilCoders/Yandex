package ru.yandex.ci.core.guava;

import com.google.common.util.concurrent.Service;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class ShutdownServiceListener extends Service.Listener {
    private static final Logger log = LoggerFactory.getLogger(ShutdownServiceListener.class);

    private final String serviceName;

    public ShutdownServiceListener(String serviceName) {
        this.serviceName = serviceName;
    }

    @Override
    public void starting() {
        log.info("Service {} is starting...", serviceName);
    }

    @Override
    public void running() {
        log.info("Service {} is running", serviceName);
    }

    @Override
    public void stopping(Service.State from) {
        log.info("Service {} is stopping... Previous state {}", serviceName, from);
    }

    @Override
    public void terminated(Service.State from) {
        log.info("Service {} is terminated. Previous state {}", serviceName, from);
    }

    @Override
    public void failed(Service.State from, Throwable failure) {
        if (from == Service.State.STARTING) {
            log.error("Service {} failed to start. Exiting...", serviceName, failure);
            System.exit(1);
        }
    }
}
