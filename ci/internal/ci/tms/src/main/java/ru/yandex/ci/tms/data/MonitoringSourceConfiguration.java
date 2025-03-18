package ru.yandex.ci.tms.data;

import java.util.List;
import java.util.Set;

import lombok.Value;

import ru.yandex.ci.client.juggler.model.ChecksStatusRequest;
import ru.yandex.ci.client.juggler.model.Status;

@Value
public class MonitoringSourceConfiguration {
    List<SolomonAlert> majorSolomonAlerts;
    Set<SolomonAlert.Status> majorTriggerStatuses;
    ChecksStatusRequest majorJugglerChecks;
    ChecksStatusRequest blockingJugglerChecks;
    Set<Status> majorJugglerTriggerStatuses;
    Set<Status> majorJugglerHoldStateStatuses;
    SolomonAlert inflightPrecommitsAlert;
}
