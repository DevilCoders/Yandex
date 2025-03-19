package yandex.cloud.dashboard.model.spec.panel;

import com.fasterxml.jackson.annotation.JsonSubTypes;
import com.fasterxml.jackson.annotation.JsonTypeInfo;
import yandex.cloud.dashboard.model.spec.Spec;
import yandex.cloud.dashboard.model.spec.generic.GraphParamsSpec;

/**
 * @author ssytnik
 */
@JsonTypeInfo(use = JsonTypeInfo.Id.NAME, property = "type")
@JsonSubTypes({
        @JsonSubTypes.Type(value = DashboardListSpec.class, name = "dashlist"),
        @JsonSubTypes.Type(value = GraphSpec.class, name = "graph"),
        @JsonSubTypes.Type(value = PlaceholderSpec.class, name = "placeholder"),
        @JsonSubTypes.Type(value = SinglestatSpec.class, name = "singlestat")
})
public interface PanelSpec extends Spec {

    String getTitle();

    String getDescription();

    GraphParamsSpec getParams();

    PanelSpec withParams(GraphParamsSpec params);
}
