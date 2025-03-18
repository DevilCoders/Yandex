package ru.yandex.ci.tms.spring.tasks.autocheck.degradation;

import java.util.Arrays;
import java.util.EnumSet;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.core.env.Environment;

import ru.yandex.ci.client.juggler.JugglerClient;
import ru.yandex.ci.client.juggler.model.ChecksStatusRequest;
import ru.yandex.ci.client.juggler.model.FilterRequest;
import ru.yandex.ci.client.juggler.model.Status;
import ru.yandex.ci.tms.client.SolomonClient;
import ru.yandex.ci.tms.data.MonitoringSourceConfiguration;
import ru.yandex.ci.tms.data.SolomonAlert;
import ru.yandex.ci.tms.task.autocheck.degradation.AutocheckDegradationMonitoringsSource;
import ru.yandex.ci.tms.task.autocheck.degradation.AutocheckDegradationStateKeeper;
import ru.yandex.ci.tms.task.autocheck.degradation.AutomaticDegradation;
import ru.yandex.ci.util.SpringParseUtils;

@Configuration
@Import({
        JugglerClientConfig.class,
        SolomonClientConfig.class,
        AutocheckDegradationCommonConfig.class
})

public class AutocheckDegradationMonitoringsSourceConfig {

    @Bean
    AlertsConfiguration majorAlertsConfiguration(
            @Value("${ci.majorAlertsConfiguration.alertsMajorProjectIds}") String[] alertsMajorProjectIds,
            Environment env
    ) {
        return alertsConfiguration("ci.majorAlertsConfiguration.", List.of(alertsMajorProjectIds), env);
    }

    @Bean
    public List<SolomonAlert> solomonAlerts(AlertsConfiguration alertsConfiguration) {
        return alertsConfiguration.getProjectId2AlertIds().entrySet().stream()
                .flatMap(entry -> entry.getValue().stream().map(alertId -> new SolomonAlert(entry.getKey(), alertId)))
                .collect(Collectors.toList());
    }

    @Bean("major_trigger")
    public ChecksStatusRequest majorChecksStatusRequest(
            @Value("${ci.majorChecksStatusRequest.host}") String host,
            @Value("${ci.majorChecksStatusRequest.namespace}") String namespace,
            @Value("${ci.majorChecksStatusRequest.service}") String service,
            @Value("${ci.majorChecksStatusRequest.tags}") String tags
    ) {
        return createChecksStatusRequest(host, namespace, service, tags);
    }

    @Bean
    public MonitoringSourceConfiguration monitoringSourceConfiguration(
            AlertsConfiguration majorAlertsConfig,
            List<SolomonAlert> solomonAlerts,
            @Qualifier("major_trigger") ChecksStatusRequest majorJugglerChecks,
            @Value("${ci.monitoringSourceConfiguration.inflightPrecommitsAlertProjectId}")
                    String inflightPrecommitsAlertProjectId,
            @Value("${ci.monitoringSourceConfiguration.inflightPrecommitsAlertId}") String inflightPrecommitsAlertId,
            @Qualifier("trigger_blocking") ChecksStatusRequest blockingJugglerChecks,
            @Value("${ci.monitoringSourceConfiguration.majorJugglerTriggerStatuses}")
                    Status[] majorJugglerTriggerStatuses,
            @Value("${ci.monitoringSourceConfiguration.majorJugglerHoldStateStatuses}")
                    Status[] majorJugglerHoldStateStatuses
    ) {
        return new MonitoringSourceConfiguration(
                solomonAlerts,
                majorAlertsConfig.getTriggerStatuses(),
                majorJugglerChecks,
                blockingJugglerChecks,
                Set.of(majorJugglerTriggerStatuses),
                Set.of(majorJugglerHoldStateStatuses),
                new SolomonAlert(inflightPrecommitsAlertProjectId, inflightPrecommitsAlertId)
        );
    }

    @Bean("trigger_blocking")
    public ChecksStatusRequest blockingChecksStatusRequest(
            @Value("${ci.blockingChecksStatusRequest.host}") String host,
            @Value("${ci.blockingChecksStatusRequest.namespace}") String namespace,
            @Value("${ci.blockingChecksStatusRequest.service}") String service,
            @Value("${ci.blockingChecksStatusRequest.tags}") String tags
    ) {
        return createChecksStatusRequest(host, namespace, service, tags);
    }

    @Bean
    public AutocheckDegradationMonitoringsSource degradationMonitoringsSource(
            MonitoringSourceConfiguration monitoringSourceConfiguration,
            SolomonClient solomonClient,
            JugglerClient jugglerClient,
            AutocheckDegradationStateKeeper autocheckDegradationStateKeeper
    ) {
        return new AutocheckDegradationMonitoringsSource(
                monitoringSourceConfiguration,
                autocheckDegradationStateKeeper,
                solomonClient,
                jugglerClient
        );
    }

    private AlertsConfiguration alertsConfiguration(
            String alertsPropertiesPrefix,
            List<String> projectIds,
            Environment env
    ) {
        Map<String, List<String>> projectId2AlertIds = new HashMap<>();

        for (String projectId : projectIds) {
            try {
                List<String> alertIds = List.of(
                        env.getProperty(alertsPropertiesPrefix + projectId + ".ids").split(",")
                );

                if (!alertIds.isEmpty()) {
                    projectId2AlertIds.put(projectId, alertIds);
                }
            } catch (NullPointerException ex) {
                throw new IllegalStateException(String.format("Property '%s' value must not be null",
                        alertsPropertiesPrefix + projectId + ".ids"));
            }
        }

        String rawTriggerStatus = env.getProperty(alertsPropertiesPrefix + "triggerStatus");

        try {
            Set<SolomonAlert.Status> triggerStatus = Arrays.stream(rawTriggerStatus.split(","))
                    .map(rawValue -> SolomonAlert.Status.valueOf(rawValue.toUpperCase()))
                    .collect(Collectors.toCollection(() -> EnumSet.noneOf(SolomonAlert.Status.class)));

            return new AlertsConfiguration(projectId2AlertIds, triggerStatus);
        } catch (IllegalArgumentException ex) {
            throw new IllegalStateException(String.format("Property '%s' value must be list of %s, got %s",
                    alertsPropertiesPrefix + ".triggerStatus", Arrays.toString(SolomonAlert.Status.values()),
                    rawTriggerStatus));
        } catch (NullPointerException ex) {
            throw new IllegalStateException(String.format("Property '%s' value must not be null",
                    alertsPropertiesPrefix + ".triggerStatus"));
        }
    }

    private ChecksStatusRequest createChecksStatusRequest(
            String host,
            String namespace,
            String service,
            String tagsRaw
    ) {
        if (Stream.of(host, namespace, service, tagsRaw).allMatch(String::isEmpty)) {
            return new ChecksStatusRequest(List.of(), 1);
        }

        return new ChecksStatusRequest(
                List.of(
                        new FilterRequest(host, namespace, service, SpringParseUtils.parseToStringList(tagsRaw))
                ),
                AutomaticDegradation.getLimitDefault()
        );
    }

    public static class AlertsConfiguration {
        private final Map<String, List<String>> projectId2AlertIds;
        private final Set<SolomonAlert.Status> triggerStatuses;

        AlertsConfiguration(Map<String, List<String>> projectId2AlertIds, Set<SolomonAlert.Status> triggerStatuses) {
            this.projectId2AlertIds = Map.copyOf(projectId2AlertIds);
            this.triggerStatuses = Set.copyOf(triggerStatuses);
        }

        public Map<String, List<String>> getProjectId2AlertIds() {
            return projectId2AlertIds;
        }

        public Set<SolomonAlert.Status> getTriggerStatuses() {
            return triggerStatuses;
        }
    }
}
