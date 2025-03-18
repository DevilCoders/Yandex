package ru.yandex.ci.common.ydb;

import io.prometheus.client.Collector;
import io.prometheus.client.CollectorRegistry;
import lombok.extern.slf4j.Slf4j;

import yandex.cloud.repository.db.TxManagerImpl;

@Slf4j
public class PrometheusMetricsFilter {

    private PrometheusMetricsFilter() {
        //
    }

    static void disableTxMetrics() {
        log.info("Disabling Prometheus metrics from ORM");
        try {
            for (var field : TxManagerImpl.class.getDeclaredFields()) {
                if (Collector.class.isAssignableFrom(field.getType())) {
                    field.setAccessible(true);
                    var collector = (Collector) field.get(null);
                    log.info("Disabling {}", field.getName());
                    CollectorRegistry.defaultRegistry.unregister(collector);
                }
            }
        } catch (Exception e) {
            throw new RuntimeException("Unable to access " + TxManagerImpl.class + "fields to disable Prometheus");
        }
    }
}
