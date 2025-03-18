package ru.yandex.monlib.metrics;

import java.util.EnumMap;

import org.apache.log4j.AppenderSkeleton;
import org.apache.log4j.spi.LoggingEvent;

import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.primitives.Rate;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

/**
 * Produced metrics:
 * RATE log4j.events{level=WARN|CRIT|etc}
 * RATE log4j.chars{}
 *
 * @author Vladimir Gordiychuk
 */
public class InstrumentedAppender extends AppenderSkeleton {
    private final EnumMap<LogLevel, Rate> eventRateByLevel;
    private final Rate charsRate;

    public InstrumentedAppender() {
        this(MetricRegistry.root());
    }

    public InstrumentedAppender(MetricRegistry registry) {
        EnumMap<LogLevel, Rate> messageRateByLevel = new EnumMap<>(LogLevel.class);
        for (LogLevel level : LogLevel.values()) {
            Rate rate = registry.rate("log4j.events", Labels.of("level", level.name()));
            messageRateByLevel.put(level, rate);
        }
        this.eventRateByLevel = messageRateByLevel;

        this.charsRate = registry.rate("log4j.chars");
    }

    @Override
    protected void append(LoggingEvent event) {
        LogLevel level = LogLevel.valueOf(event.getLevel());
        eventRateByLevel.get(level).inc();

        String message = event.getRenderedMessage();
        if (message != null) {
            charsRate.add(message.length());
        }
    }

    @Override
    public void close() {
    }

    @Override
    public boolean requiresLayout() {
        return false;
    }
}
