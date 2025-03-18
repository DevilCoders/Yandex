package ru.yandex.ci.common.temporal.logging;

import java.time.Duration;
import java.time.Instant;
import java.time.ZoneId;
import java.time.format.DateTimeFormatter;
import java.util.List;
import java.util.Map;
import java.util.Objects;

import javax.annotation.Nullable;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.common.annotations.VisibleForTesting;
import com.google.common.collect.Lists;
import lombok.experimental.Delegate;
import vtail.api.core.Log;

import ru.yandex.ci.client.logs.LogsClient;
import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.common.temporal.TemporalService;

public class TemporalLogService {

    public static final int LINES_LIMIT = 10000;

    //Add small extra time shift for logging request in case of time desync
    @VisibleForTesting
    static final Duration TIME_SHIFT = Duration.ofHours(1);

    private static final ObjectMapper CUSTOM_FIELDS_MAPPER = new ObjectMapper();
    private static final DateTimeFormatter DEFAULT_FORMATTER = DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss.SSS");

    private final TemporalService temporalService;
    private final LogsClient logsClient;
    private final String logProject;
    private final String logsService;
    private final String logsEnvironment;
    private final DateTimeFormatter dateTimeFormatter;

    public TemporalLogService(
            TemporalService temporalService,
            LogsClient logsClient,
            String logProject,
            String logsService,
            String logsEnvironment,
            ZoneId zoneId
    ) {
        this.temporalService = temporalService;
        this.logsClient = logsClient;
        this.logProject = logProject;
        this.logsService = logsService;
        this.logsEnvironment = logsEnvironment;
        this.dateTimeFormatter = DEFAULT_FORMATTER.withZone(zoneId);
    }

    @Nullable
    public String getLog(String workflowId) {
        var executionInfo = temporalService.getWorkflowExecutionInfo(workflowId);
        var startTime = ProtoConverter.convert(executionInfo.getStartTime()).minus(TIME_SHIFT);

        var endTime = Instant.now();
        if (executionInfo.hasCloseTime()) {
            endTime = ProtoConverter.convert(executionInfo.getCloseTime());
        }
        endTime = endTime.plus(TIME_SHIFT);


        var query = Map.of(
                "project", logProject,
                "service", logsService,
                "env", logsEnvironment,
                "task_id", workflowId
        );

        var messages = logsClient.getLog(startTime, endTime, query, LINES_LIMIT);
        return formatLogs(messages);
    }

    private String formatLogs(List<Log.LogMessage> messages) {
        if (messages.isEmpty()) {
            return "Empty logs\n";
        }

        var stringBuilder = new StringBuilder();
        addLinesLimit(stringBuilder, messages);

        LogMessage previousMessage = null;
        for (Log.LogMessage logMessage : Lists.reverse(messages)) {
            var message = new LogMessage(logMessage);
            printMetadata(stringBuilder, message, previousMessage);
            formatLine(stringBuilder, message);
            previousMessage = message;
        }
        return stringBuilder.toString();
    }

    private void addLinesLimit(StringBuilder stringBuilder, List<Log.LogMessage> messages) {
        if (messages.size() == LINES_LIMIT) {
            stringBuilder.append("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
            stringBuilder.append("Log is too big. Showing last ").append(LINES_LIMIT).append(" lines\n");
            stringBuilder.append("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n");
        }
    }

    private void printMetadata(
            StringBuilder stringBuilder,
            LogMessage message,
            @Nullable LogMessage previousMessage
    ) {

        if (!needToUpdateMetadata(message, previousMessage)) {
            return;
        }

        stringBuilder.append("\n===========================================================\n");
        stringBuilder.append(message.getFormattedDate()).append("\n");
        stringBuilder.append("Activity ID: ").append(message.getActivityId()).append("\n");
        stringBuilder.append("Run ID: ").append(message.getRunId()).append("\n");
        stringBuilder.append("Host: ").append(message.getHostname()).append("\n");
        stringBuilder.append("===========================================================\n\n");
    }

    private boolean needToUpdateMetadata(LogMessage message, @Nullable LogMessage previousMessage) {
        if (previousMessage == null) {
            return true;
        }

        if (!message.getHostname().equals(previousMessage.getHostname())) {
            return true;
        }

        if (!Objects.equals(message.getActivityId(), previousMessage.getActivityId())) {
            return true;
        }

        if (!Objects.equals(message.getRunId(), previousMessage.getRunId())) {
            return true;
        }
        return false;
    }

    private void formatLine(StringBuilder stringBuilder, LogMessage message) {
        stringBuilder.append(message.getFormattedDate()).append(" ")
                .append(message.getLevel()).append(" ")
                .append("[").append(message.getShortComponent()).append("] ")
                .append(message.getMessage())
                .append("\n");
    }

    private static String toShortComponent(String component) {
        int lastDotIndex = component.lastIndexOf('.');
        if (lastDotIndex <= 0) {
            return component;
        }
        return component.substring(lastDotIndex + 1);
    }

    private class LogMessage {
        @Delegate
        private final Log.LogMessage message;
        private final JsonNode customFields;

        private LogMessage(Log.LogMessage message) {
            this.message = message;
            try {
                customFields = CUSTOM_FIELDS_MAPPER.readTree(message.getRest());
            } catch (JsonProcessingException e) {
                throw new RuntimeException(e);
            }
        }

        private String getShortComponent() {
            return toShortComponent(getComponent());
        }

        private String getFormattedDate() {
            var instant = ProtoConverter.convert(message.getTime());
            return dateTimeFormatter.format(instant);
        }

        @Nullable
        private String getCustomField(String field) {
            var node = customFields.get(field);
            return node == null
                    ? null
                    : node.asText();
        }

        @Nullable
        private String getActivityId() {
            return getCustomField(TemporalLogConstants.ACTIVITY_ID);
        }

        @Nullable
        private String getRunId() {
            return getCustomField(TemporalLogConstants.RUN_ID);
        }
    }
}
