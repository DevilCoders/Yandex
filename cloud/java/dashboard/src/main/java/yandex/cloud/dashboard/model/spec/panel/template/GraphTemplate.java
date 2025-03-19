package yandex.cloud.dashboard.model.spec.panel.template;

import com.fasterxml.jackson.annotation.JsonSubTypes;
import com.fasterxml.jackson.annotation.JsonTypeInfo;
import yandex.cloud.dashboard.model.spec.panel.GraphSpec;

/**
 * @author girevoyt
 */
@JsonTypeInfo(use = JsonTypeInfo.Id.NAME, property = "name")
@JsonSubTypes({
        @JsonSubTypes.Type(value = RpsTemplate.class, name = "rps"),
        @JsonSubTypes.Type(value = ErrorsTemplate.class, name = "errors"),
        @JsonSubTypes.Type(value = PercentileTemplate.class, name = "percentile"),
        @JsonSubTypes.Type(value = AliasTemplate.class, name = "alias"),
        @JsonSubTypes.Type(value = PatchSelectTemplate.class, name = "patchSelect")
})
public interface GraphTemplate extends Template<GraphSpec, GraphTemplate> {
}
