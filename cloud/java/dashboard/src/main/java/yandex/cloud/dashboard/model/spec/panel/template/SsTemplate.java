package yandex.cloud.dashboard.model.spec.panel.template;

import com.fasterxml.jackson.annotation.JsonSubTypes;
import com.fasterxml.jackson.annotation.JsonTypeInfo;
import yandex.cloud.dashboard.model.spec.panel.SinglestatSpec;

/**
 * @author girevoyt
 */
@JsonTypeInfo(use = JsonTypeInfo.Id.NAME, property = "name")
@JsonSubTypes({
        @JsonSubTypes.Type(value = TrafficLightsTemplate.class, name = "trafficlights")
})
public interface SsTemplate extends Template<SinglestatSpec, SsTemplate> {
}
