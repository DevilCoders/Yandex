package yandex.cloud.dashboard.model.spec.panel;

import com.google.common.base.Preconditions;
import lombok.Value;
import lombok.With;
import yandex.cloud.dashboard.model.spec.generic.GraphParamsSpec;
import yandex.cloud.dashboard.model.spec.validator.SpecValidationContext;

/**
 * @author entropia
 */
@Value
@With
public class PlaceholderSpec implements PanelSpec {
    String description;
    GraphParamsSpec params;

    @Override
    public String getTitle() {
        return null;
    }


    @Override
    public void validate(SpecValidationContext context) {
        Preconditions.checkArgument(params == null || params.isJustSize(),
                "Placeholder panel should specify size parameters only, but %s is found", params);
    }
}
