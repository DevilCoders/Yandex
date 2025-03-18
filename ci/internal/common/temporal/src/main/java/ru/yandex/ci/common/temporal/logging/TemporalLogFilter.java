package ru.yandex.ci.common.temporal.logging;

import io.temporal.internal.logging.LoggerTag;
import org.apache.logging.log4j.core.Filter;
import org.apache.logging.log4j.core.LogEvent;
import org.apache.logging.log4j.core.config.Node;
import org.apache.logging.log4j.core.config.plugins.Plugin;
import org.apache.logging.log4j.core.config.plugins.PluginFactory;
import org.apache.logging.log4j.core.filter.AbstractFilter;

@Plugin(name = "TemporalFilter", category = Node.CATEGORY, elementType = Filter.ELEMENT_TYPE, printObject = true)
public class TemporalLogFilter extends AbstractFilter {
    private TemporalLogFilter() {
    }

    @Override
    public Result filter(LogEvent event) {
        return event.getContextData().containsKey(LoggerTag.WORKFLOW_ID) ? Result.ACCEPT : Result.DENY;
    }

    @PluginFactory
    public static TemporalLogFilter createFilter() {
        return new TemporalLogFilter();
    }
}
