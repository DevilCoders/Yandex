package ru.yandex.ci.core.logbroker;

import java.util.concurrent.TimeUnit;

import org.springframework.context.SmartLifecycle;

import ru.yandex.kikimr.persqueue.proxy.ProxyBalancer;

public class LogbrokerProxyBalancerHolder implements SmartLifecycle {

    private final ProxyBalancer proxyBalancer;
    private boolean running;

    public LogbrokerProxyBalancerHolder(ProxyBalancer proxyBalancer) {
        this.proxyBalancer = proxyBalancer;
    }

    public ProxyBalancer getProxyBalancer() {
        return proxyBalancer;
    }

    @Override
    public void start() {
        running = true;
    }

    @Override
    public void stop() {
        running = false;

        try {
            proxyBalancer.shutdown(15, TimeUnit.SECONDS);
        } catch (InterruptedException e) {
            throw new RuntimeException("Unable to shutdown balancer", e);
        }
    }

    @Override
    public boolean isRunning() {
        return running;
    }

}
