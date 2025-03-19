package yandex.cloud.dashboard.model.spec.panel.template;

import lombok.Value;
import lombok.With;
import yandex.cloud.dashboard.model.spec.panel.DisplaySpec;
import yandex.cloud.dashboard.model.spec.panel.GraphSpec;
import yandex.cloud.dashboard.model.spec.panel.QuerySpec;
import yandex.cloud.dashboard.model.spec.panel.SensorValueType;
import yandex.cloud.dashboard.model.spec.panel.YAxisInListSpec;

import java.util.Collections;
import java.util.List;

import static com.google.common.base.MoreObjects.firstNonNull;
import static java.util.stream.Collectors.toList;
import static yandex.cloud.dashboard.model.spec.Constants.FORMAT_SHORT;
import static yandex.cloud.dashboard.model.spec.panel.SensorValueType.UseCase.VALUE;
import static yandex.cloud.dashboard.model.spec.panel.SensorValueType.counter;
import static yandex.cloud.dashboard.util.Mergeable.mergeNullable;

/**
 * @author ssytnik
 */
@With
@Value
public class ErrorsTemplate implements GraphTemplate {
    SumLinesSpec sumLines;
    SensorValueType sensor;

    @Override
    public GraphSpec transform(GraphSpec source) {
        return source.toBuilder()
                .display(mergeNullable(source.getDisplay(), new DisplaySpec(null, 0, null, null, null, null, null, null, null)))
                .queries(source.getQueries().stream().map(this::query).collect(toList()))
                .yAxes(mergeNullable(source.getYAxes(), Collections.nCopies(2, new YAxisInListSpec(1, FORMAT_SHORT, null, null, null, "0"))))
                .build();
    }

    private QuerySpec query(QuerySpec source) {
        return source.toBuilder()
                .groupByTime(sensor().groupByTime(VALUE))
                .select(sensor().selectBuilder(VALUE)
                        .addCallIf(sumLines().isEnabled(), sumLines().getCallFn(), sumLines().getCallParams())
                        .build())
                .build();
    }

    @Override
    public List<GraphTemplate> successiveTemplates() {
        return sumLines().isSingleLine() ? List.of(AliasTemplate.ALIASES_FROM_DRAW) : List.of();
    }

    public SumLinesSpec sumLines() {
        return firstNonNull(sumLines, SumLinesSpec.SINGLE_LINE);
    }

    private SensorValueType sensor() {
        return firstNonNull(sensor, counter);
    }
}
