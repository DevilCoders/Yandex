package yandex.cloud.dashboard.model.spec.generic;

import lombok.Value;
import lombok.With;
import yandex.cloud.dashboard.model.spec.Spec;
import yandex.cloud.dashboard.util.Mergeable;

import static yandex.cloud.dashboard.util.Mergeable.mergeNullable;
import static yandex.cloud.dashboard.util.ObjectUtils.firstNonNullOrNull;

/**
 * @author ssytnik
 */
@With
@Value
public class QueryParamsSpec implements Spec, Mergeable<QueryParamsSpec> {
    LabelsSpec labels;
    String defaultTimeWindow;
    Boolean dropNan;

    @Override
    public QueryParamsSpec merge(QueryParamsSpec lowerPrecedence) {
        return new QueryParamsSpec(
                mergeNullable(labels, lowerPrecedence.labels),
                firstNonNullOrNull(defaultTimeWindow, lowerPrecedence.defaultTimeWindow),
                firstNonNullOrNull(dropNan, lowerPrecedence.dropNan)
        );
    }
}
