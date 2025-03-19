package yandex.cloud.dashboard.model.spec.panel;

import com.google.common.base.Preconditions;
import lombok.Value;
import lombok.With;
import yandex.cloud.dashboard.model.spec.Spec;
import yandex.cloud.dashboard.model.spec.generic.GraphParamsSpec;
import yandex.cloud.dashboard.model.spec.validator.SpecValidationContext;

import java.util.List;

/**
 * @author ssytnik
 */
@With
@Value
public class DashboardListSpec implements PanelSpec {
    String title;
    String description;
    GraphParamsSpec params;

    Boolean headings;
    Integer limit;

    Boolean recent;
    Boolean starred;
    SearchSpec search;

    @With
    @Value
    public static class SearchSpec implements Spec {
        String query;
        List<String> tags;
        Integer folderId;
    }


    @Override
    public void validate(SpecValidationContext context) {
        Preconditions.checkArgument(params == null || params.isJustSize(),
                "Dashboard list panel should specify size parameters only, but %s is found", params);
    }
}
