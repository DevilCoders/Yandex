package ru.yandex.ci.tms.task.autocheck.degradation;

import java.util.List;
import java.util.Set;
import java.util.function.Supplier;

import javax.annotation.Nullable;

import lombok.ToString;

import ru.yandex.ci.client.juggler.JugglerClient;
import ru.yandex.ci.client.juggler.model.ChecksStatusRequest;
import ru.yandex.ci.client.juggler.model.Status;
import ru.yandex.ci.tms.client.SolomonClient;
import ru.yandex.ci.tms.data.MonitoringSourceConfiguration;
import ru.yandex.ci.tms.data.SolomonAlert;

public class AutocheckDegradationMonitoringsSource {

    // Degradation triggers
    private final List<SolomonAlert> majorSolomonAlerts;
    private final Set<SolomonAlert.Status> majorTriggerStatuses;

    private final ChecksStatusRequest majorJugglerChecks;
    private final ChecksStatusRequest blockingJugglerChecks;
    private final Set<Status> majorJugglerTriggerStatuses;
    private final Set<Status> majorJugglerHoldStateStatuses;

    private final SolomonAlert inflightPrecommitsAlert;

    private final AutocheckDegradationStateKeeper stateKeeper;

    //Clients
    private final SolomonClient solomonClient;
    private final JugglerClient jugglerClient;

    public AutocheckDegradationMonitoringsSource(MonitoringSourceConfiguration configuration,
                                                 AutocheckDegradationStateKeeper stateKeeper,
                                                 SolomonClient solomonClient,
                                                 JugglerClient jugglerClient) {
        this.majorTriggerStatuses = Set.copyOf(configuration.getMajorTriggerStatuses());
        this.majorSolomonAlerts = List.copyOf(configuration.getMajorSolomonAlerts());

        this.majorJugglerChecks = configuration.getMajorJugglerChecks();
        this.blockingJugglerChecks = configuration.getBlockingJugglerChecks();
        this.majorJugglerTriggerStatuses = Set.copyOf(configuration.getMajorJugglerTriggerStatuses());
        this.majorJugglerHoldStateStatuses = Set.copyOf(configuration.getMajorJugglerHoldStateStatuses());
        this.inflightPrecommitsAlert = configuration.getInflightPrecommitsAlert();

        this.stateKeeper = stateKeeper;
        this.solomonClient = solomonClient;
        this.jugglerClient = jugglerClient;
    }

    public MonitoringState getMonitoringState() {
        return new MonitoringState(isMonitoringsTriggered(), getAutomaticDegradationState(), getInflightPrecommits());
    }

    public void savePreviousTriggerState(boolean trigger) {
        stateKeeper.setPreviousTriggerState(trigger);
    }

    private boolean isMonitoringsTriggered() {
        var jugglerChecksStatus = fetchJugglerChecksStatuses(majorJugglerChecks);
        boolean majorJugglerChecksTrigger = jugglerChecksStatus.stream()
                .anyMatch(majorJugglerTriggerStatuses::contains);

        boolean solomonAlertsTrigger = majorSolomonAlerts.stream()
                .map(this::fetchSolomonAlertStatusCode)
                .anyMatch(majorTriggerStatuses::contains);

        boolean trigger = solomonAlertsTrigger || majorJugglerChecksTrigger;

        if (!trigger && jugglerChecksStatus.stream().anyMatch(majorJugglerHoldStateStatuses::contains)) {
            trigger = stateKeeper.getPreviousTriggerState();
        }

        return trigger;
    }

    /**
     * Автоматика может быть выключена, если, например, кол-во inflight проверок ниже какой-то границы.
     * Когда кол-во inflight проверок > limit, алерт
     * https://juggler.yandex-team.ru/raw_events/
     * ?query=project%3Ddevtools.autocheck%26tag%3Dautocheck_blocking_degradation</a>
     * находится в статусе CRIT
     */
    private AutomaticDegradation getAutomaticDegradationState() {
        // См. описание в https://st.yandex-team.ru/CI-2320#60ed9661e5dbf4126438e7df
        var checksStatus = fetchJugglerChecksStatuses(blockingJugglerChecks);
        if (checksStatus.isEmpty()) {
            return AutomaticDegradation.enabled();
        }
        for (var status : checksStatus) {
            if (status == Status.WARN) {
                throw new AutomaticDegradationStoppedException("Invalid check status: %s requested for %s"
                        .formatted(status, blockingJugglerChecks));
            }
            if (status != Status.OK) {
                return AutomaticDegradation.enabled();
            }
        }
        return AutomaticDegradation.disabled();
    }

    @Nullable
    private Integer getInflightPrecommits() {
        var status = wrapWithTryCatch(() -> solomonClient.getAlertStatus(inflightPrecommitsAlert),
                "fetching solomon alert " + inflightPrecommitsAlert);
        String inflight = status.getAnnotations().get("inflight_value");
        if (inflight == null || inflight.isBlank()) {
            return null;
        }

        return Math.round(Float.parseFloat(inflight));
    }

    private List<Status> fetchJugglerChecksStatuses(ChecksStatusRequest majorJugglerChecks) {
        return wrapWithTryCatch(() -> jugglerClient.getChecksStatus(majorJugglerChecks),
                "fetching juggler checks status: " + majorJugglerChecks);
    }

    private SolomonAlert.Status fetchSolomonAlertStatusCode(SolomonAlert alert) {
        return wrapWithTryCatch(() -> solomonClient.getAlertStatusCode(alert),
                "fetching solomon alert " + alert);
    }

    private static <T> T wrapWithTryCatch(Supplier<T> action, String description) {
        try {
            return action.get();
        } catch (Exception e) {
            throw new AutomaticDegradationStoppedException("failed to " + description, e);
        }
    }

    @ToString(doNotUseGetters = true)
    public static class MonitoringState {
        private final boolean triggered;
        private final AutomaticDegradation automaticDegradation;
        @Nullable
        private final Integer inflightPrecommits;

        public MonitoringState(boolean triggered,
                               AutomaticDegradation automaticDegradation,
                               @Nullable Integer inflightPrecommits) {
            this.triggered = triggered;
            this.automaticDegradation = automaticDegradation;
            this.inflightPrecommits = inflightPrecommits;
        }

        public boolean isTriggered() {
            return !automaticDegradation.isDisabled() && triggered;
        }

        public AutomaticDegradation getAutomaticDegradation() {
            return automaticDegradation;
        }

        @Nullable
        public Integer getInflightPrecommits() {
            return inflightPrecommits;
        }

    }

}
