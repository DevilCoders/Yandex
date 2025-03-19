package yandex.cloud.dashboard.model.spec.panel;

import lombok.Builder;
import lombok.Value;
import lombok.With;
import yandex.cloud.dashboard.model.spec.dashboard.LinkSpec;
import yandex.cloud.dashboard.model.spec.generic.GraphParamsSpec;
import yandex.cloud.dashboard.model.spec.panel.template.SsTemplate;
import yandex.cloud.dashboard.model.spec.panel.template.TemplatedPanelSpec;
import yandex.cloud.dashboard.model.spec.panel.template.TemplatesSpec;

import java.util.List;

/**
 * @author girevoyt
 */
@Builder(toBuilder = true)
@With
@Value
public class SinglestatSpec implements TemplatedPanelSpec<SinglestatSpec, SsTemplate> {
    TemplatesSpec<SsTemplate> templates;
    String repeat;

    String title;
    String description;

    GraphParamsSpec params;
    QuerySpec query;

    SsColoringSpec coloring;
    String format;
    SsGaugeSpec gauge;
    List<LinkSpec> links;
    //    SSValueMappingsSpec valueMappings;
    SsValueSpec value;
    SsSparklineSpec sparkline;
}
