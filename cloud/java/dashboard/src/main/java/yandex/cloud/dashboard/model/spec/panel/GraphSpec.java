package yandex.cloud.dashboard.model.spec.panel;

import lombok.Builder;
import lombok.Value;
import lombok.With;
import yandex.cloud.dashboard.model.spec.dashboard.LinkSpec;
import yandex.cloud.dashboard.model.spec.generic.GraphParamsSpec;
import yandex.cloud.dashboard.model.spec.generic.QueryParamsSpec;
import yandex.cloud.dashboard.model.spec.panel.template.GraphTemplate;
import yandex.cloud.dashboard.model.spec.panel.template.RpsTemplate;
import yandex.cloud.dashboard.model.spec.panel.template.RpsTemplate.Rate;
import yandex.cloud.dashboard.model.spec.panel.template.TemplatedPanelSpec;
import yandex.cloud.dashboard.model.spec.panel.template.TemplatesSpec;

import java.util.List;

import static com.google.common.base.MoreObjects.firstNonNull;
import static yandex.cloud.dashboard.util.ObjectUtils.filterAndCast;

/**
 * @author ssytnik
 */
@Builder(toBuilder = true)
@With
@Value
public class GraphSpec implements TemplatedPanelSpec<GraphSpec, GraphTemplate> {
    private static final String PLACEHOLDER_LABEL = "--";

    QueryParamsSpec queryDefaults;

    TemplatesSpec<GraphTemplate> templates;
    String repeat; // TODO support multiple repeat variables
    String uiRepeat;

    String title;
    String description;
    DisplaySpec display;
    List<DrawSpec> draw;
    List<YAxisInListSpec> yAxes;
    List<LinkSpec> links;
    List<LinkSpec> dataLinks;

    GraphParamsSpec params;
    List<QuerySpec> queries;

    public boolean isPlaceholderSpec() {
        return queries.stream()
                .filter(qs -> qs != null && qs.getParams() != null && qs.getParams().getLabels() != null)
                .flatMap(qs -> qs.getParams().getLabels().getTags().values().stream())
                .anyMatch(PLACEHOLDER_LABEL::equals);
    }

    public boolean hasUiRate() {
        return templates != null && firstNonNull(templates.getList(), List.of()).stream()
                .flatMap(t -> filterAndCast(t, RpsTemplate.class))
                .anyMatch(rt -> rt.rate() == Rate.ui);
    }
}
