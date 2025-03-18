package ru.yandex.ci.client.infra.event;

import java.util.Map;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonAlias;
import lombok.Builder;
import lombok.Value;

@Value
@Builder
public class EventDto {

    Long id;

    @JsonAlias("environment_id")
    long environmentId;

    @JsonAlias("service_id")
    long serviceId;
    String title;

    @Nullable
    String description;

    @JsonAlias("start_time")
    @Nullable
    Long startTime;

    @JsonAlias("finish_time")
    @Nullable
    Long finishTime;

    @Nullable
    Long duration;

    TypeDto type;

    SeverityDto severity;

    @Nullable
    String tickets;

    @Nullable
    Map<String, Object> meta;

    @JsonAlias("send_email_notifications")
    @Nullable
    Boolean sendEmailNotifications;

    @JsonAlias("set_all_available_dc")
    @Nullable
    Boolean setAllAvailableDc;

    @Nullable
    Boolean man;
    @Nullable
    Boolean myt;
    @Nullable
    Boolean sas;
    @Nullable
    Boolean vla;
    @Nullable
    Boolean vlx;
    @Nullable
    Boolean iva;

}
