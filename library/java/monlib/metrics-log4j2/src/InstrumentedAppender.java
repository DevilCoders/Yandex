package ru.yandex.monlib.metrics.log4j2;

import java.io.Serializable;
import java.util.HashMap;
import java.util.Map;

import org.apache.logging.log4j.Level;
import org.apache.logging.log4j.core.Appender;
import org.apache.logging.log4j.core.Core;
import org.apache.logging.log4j.core.Filter;
import org.apache.logging.log4j.core.Layout;
import org.apache.logging.log4j.core.LogEvent;
import org.apache.logging.log4j.core.appender.AbstractAppender;
import org.apache.logging.log4j.core.config.plugins.Plugin;
import org.apache.logging.log4j.core.config.plugins.PluginAttribute;
import org.apache.logging.log4j.core.config.plugins.PluginElement;
import org.apache.logging.log4j.core.config.plugins.PluginFactory;
import org.apache.logging.log4j.message.Message;

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
@Plugin(name = "Metrics", category = Core.CATEGORY_NAME, elementType = Appender.ELEMENT_TYPE, printObject = true)
public class InstrumentedAppender extends AbstractAppender {
    private final Map<Level, Rate> eventRateByLevel;
    private final Rate charsRate;

    public InstrumentedAppender(
            MetricRegistry registry,
            String name,
            Filter filter,
            Layout<? extends Serializable> layout,
            boolean ignoreExceptions)
    {
        super(name, filter, layout, ignoreExceptions);
        MetricRegistry subRegistry = registry.subRegistry("appender", name);
        Map<Level, Rate> messageRateByLevel = new HashMap<>();
        for (Level level : Level.values()) {
            Rate rate = subRegistry.rate("log4j.events", Labels.of("level", level.name()));
            messageRateByLevel.put(level, rate);
        }
        this.eventRateByLevel = messageRateByLevel;
        this.charsRate = subRegistry.rate("log4j.chars");
    }

    /**
     * Create a new instance of the appender using the global spectator registry.
     */
    @PluginFactory
    public static InstrumentedAppender createAppender(
            @PluginAttribute("name") String name,
            @PluginAttribute("ignoreExceptions") boolean ignoreExceptions,
            @PluginElement("Layout") Layout<? extends Serializable> layout,
            @PluginElement("Filters") Filter filter)
    {
        return new InstrumentedAppender(MetricRegistry.root(), name, filter, layout, ignoreExceptions);
    }

    @Override
    public void append(LogEvent event) {
        eventRateByLevel.get(event.getLevel()).inc();

        Message message = event.getMessage();
        if (message != null) {
            charsRate.add(message.getFormattedMessage().length());
        }
    }
}
