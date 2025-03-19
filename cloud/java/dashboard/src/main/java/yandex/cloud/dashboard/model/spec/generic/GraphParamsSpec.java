package yandex.cloud.dashboard.model.spec.generic;

import com.google.common.base.Preconditions;
import lombok.Value;
import lombok.With;
import yandex.cloud.dashboard.model.spec.Spec;
import yandex.cloud.dashboard.util.Mergeable;

import static yandex.cloud.dashboard.util.ObjectUtils.firstNonNullOrNull;

/**
 * @author ssytnik
 */
@With
@Value
public class GraphParamsSpec implements Spec, Mergeable<GraphParamsSpec> {
    String datasource;
    Integer width;
    Integer height;

    @Override
    public GraphParamsSpec merge(GraphParamsSpec lowerPrecedence) {
        return new GraphParamsSpec(
                firstNonNullOrNull(datasource, lowerPrecedence.datasource),
                firstNonNullOrNull(width, lowerPrecedence.width),
                firstNonNullOrNull(height, lowerPrecedence.height)
        );
    }

    // TODO panel size is applicable to any panel (not only graph) and thus, should be extracted to a separate spec class
    public boolean isJustSize() {
        return datasource == null;
    }

    private static void validateResolvedCommon(GraphParamsSpec params) {
        Preconditions.checkNotNull(params, "Graph params should be specified at lease once at dashboard, row or panel level");
    }

    public static void validateResolveDatasource(GraphParamsSpec params) {
        validateResolvedCommon(params);
        Preconditions.checkNotNull(params.getDatasource(), "Merged graph params should contain datasource");
    }

    public static void validateResolvedSize(GraphParamsSpec params) {
        validateResolvedCommon(params);
        Preconditions.checkNotNull(params.getWidth(), "Merged graph params should contain width");
        Preconditions.checkNotNull(params.getHeight(), "Merged graph params should contain height");
    }
}
