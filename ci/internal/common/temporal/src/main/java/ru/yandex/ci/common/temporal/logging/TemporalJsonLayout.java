package ru.yandex.ci.common.temporal.logging;

import java.nio.charset.StandardCharsets;
import java.time.Instant;
import java.time.format.DateTimeFormatter;
import java.util.UUID;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;
import com.google.common.annotations.VisibleForTesting;
import com.google.common.base.Preconditions;
import com.google.common.base.Strings;
import io.temporal.internal.logging.LoggerTag;
import org.apache.logging.log4j.core.Layout;
import org.apache.logging.log4j.core.LogEvent;
import org.apache.logging.log4j.core.config.Node;
import org.apache.logging.log4j.core.config.plugins.Plugin;
import org.apache.logging.log4j.core.config.plugins.PluginAttribute;
import org.apache.logging.log4j.core.config.plugins.PluginFactory;
import org.apache.logging.log4j.core.layout.AbstractStringLayout;

import ru.yandex.ci.common.application.CiApplication;
import ru.yandex.ci.util.HostnameUtils;

/**
 * Format - https://docs.yandex-team.ru/classifieds-infra/conventions/logs
 */
@Plugin(name = "TemporalJsonLayout", category = Node.CATEGORY, elementType = Layout.ELEMENT_TYPE, printObject = true)
public class TemporalJsonLayout extends AbstractStringLayout {

    private static final ObjectMapper MAPPER = new ObjectMapper();
    private static final DateTimeFormatter FORMATTER = DateTimeFormatter.ISO_INSTANT;

    private final String environment;
    private final String service;

    private TemporalJsonLayout(String service) {
        super(StandardCharsets.UTF_8);
        environment = CiApplication.getApplicationEnvironment();
        this.service = service;
    }

    @PluginFactory
    public static TemporalJsonLayout createLayout(@PluginAttribute("service") String service) {
        Preconditions.checkArgument(!Strings.isNullOrEmpty(service), "'service' is not provided");
        return new TemporalJsonLayout(service);
    }

    @Override
    public String toSerializable(LogEvent event) {

        Preconditions.checkArgument(
                event.getContextData().containsKey(LoggerTag.WORKFLOW_ID),
                "Not a Temporal LogEvent. Check that you have applied TemporalLogFilter. %s",
                event
        );

        ObjectNode message = MAPPER.createObjectNode();

        writeMessage(event, message);

        try {
            return MAPPER.writeValueAsString(message) + "\n";
        } catch (JsonProcessingException e) {
            throw new RuntimeException(e);
        }
    }

    private void writeMessage(LogEvent event, ObjectNode message) {
        message.put("time", toTime(event.getInstant()));
        message.put("project", "ci");
        message.put("service", service);
        message.put("message", toMessage(event));
        message.put("__record_id", UUID.randomUUID().toString());
        message.put("hostname", HostnameUtils.getHostname());
        message.put("env", environment);
        message.put("level", event.getLevel().toString());
        message.put("component", event.getLoggerName());
        message.put("thread", event.getThreadName());
        addFieldIfExists(event, message, LoggerTag.WORKFLOW_ID, TemporalLogConstants.WORKFLOW_ID);
        addFieldIfExists(event, message, LoggerTag.ACTIVITY_ID, TemporalLogConstants.ACTIVITY_ID);
        addFieldIfExists(event, message, LoggerTag.RUN_ID, TemporalLogConstants.RUN_ID);
    }

    private void addFieldIfExists(LogEvent event, ObjectNode message, String contextName, String logName) {
        var value = event.getContextData().getValue(contextName);
        if (value != null) {
            message.put(logName, value.toString());
        }
    }

    private String toMessage(LogEvent event) {
        var thrownProxy = event.getThrownProxy();
        String message = event.getMessage().getFormattedMessage();
        if (thrownProxy == null) {
            return message;
        }
        return message + ": " + thrownProxy.getExtendedStackTraceAsString();
    }

    @VisibleForTesting
    String toTime(org.apache.logging.log4j.core.time.Instant log4jInstant) {
        var javaInstant = Instant.ofEpochSecond(log4jInstant.getEpochSecond(), log4jInstant.getNanoOfSecond());
        return FORMATTER.format(javaInstant);
    }

}
