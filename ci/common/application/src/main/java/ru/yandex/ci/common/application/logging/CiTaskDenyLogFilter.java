package ru.yandex.ci.common.application.logging;

import org.apache.logging.log4j.core.Filter;
import org.apache.logging.log4j.core.LogEvent;
import org.apache.logging.log4j.core.config.Node;
import org.apache.logging.log4j.core.config.plugins.Plugin;
import org.apache.logging.log4j.core.config.plugins.PluginFactory;
import org.apache.logging.log4j.core.filter.AbstractFilter;

@Plugin(name = "CiTaskDenyFilter", category = Node.CATEGORY, elementType = Filter.ELEMENT_TYPE, printObject = true)
public class CiTaskDenyLogFilter extends AbstractFilter {

    @Override
    public Filter.Result filter(LogEvent event) {
        var data = event.getContextData();
        if (data.containsKey("WorkflowId") || data.containsKey("BAZINGA")) {
            return Filter.Result.DENY;
        }
        return Filter.Result.ACCEPT;
    }

    @PluginFactory
    public static CiTaskDenyLogFilter createFilter() {
        return new CiTaskDenyLogFilter();
    }
}
