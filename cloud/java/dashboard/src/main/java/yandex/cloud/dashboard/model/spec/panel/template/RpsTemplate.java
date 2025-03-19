package yandex.cloud.dashboard.model.spec.panel.template;

import lombok.Value;
import lombok.With;
import yandex.cloud.dashboard.model.spec.Constants;
import yandex.cloud.dashboard.model.spec.panel.GraphSpec;
import yandex.cloud.dashboard.model.spec.panel.QuerySpec;
import yandex.cloud.dashboard.model.spec.panel.SensorValueType;
import yandex.cloud.dashboard.model.spec.panel.YAxisInListSpec;

import java.util.List;

import static com.google.common.base.MoreObjects.firstNonNull;
import static java.util.stream.Collectors.toList;
import static yandex.cloud.dashboard.model.spec.Constants.FORMAT_SHORT;
import static yandex.cloud.dashboard.model.spec.panel.SensorValueType.UseCase.DERIV;
import static yandex.cloud.dashboard.model.spec.panel.SensorValueType.counter;
import static yandex.cloud.dashboard.util.Mergeable.mergeNullable;

/**
 * @author ssytnik
 */
@With
@Value
public class RpsTemplate implements GraphTemplate {
    SumLinesSpec sumLines;
    Rate rate;
    SensorValueType sensor;

    @Override
    public GraphSpec transform(GraphSpec source) {
        return source.toBuilder()
                .queries(source.getQueries().stream().map(this::query).collect(toList()))
                .yAxes(mergeNullable(source.getYAxes(), List.of(new YAxisInListSpec(1, FORMAT_SHORT, rate().caption(), null, null, "0"))))
                .build();
    }

    private QuerySpec query(QuerySpec source) {
        return source.toBuilder()
                .groupByTime(sensor().groupByTime(DERIV))
                .select(sensor().selectBuilder(DERIV)
                        .addCallIf(sumLines().isEnabled(), sumLines().getCallFn(), sumLines().getCallParams())
                        .addCallIf(rate().hasMultiplier(), "math", "* " + rate().multiplier())
                        .build())
                .build();
    }

    @Override
    public List<GraphTemplate> successiveTemplates() {
        return sumLines().isSingleLine() ? List.of(new AliasTemplate(rate().caption())) : List.of();
    }


    public SumLinesSpec sumLines() {
        return firstNonNull(sumLines, SumLinesSpec.SINGLE_LINE);
    }

    public Rate rate() {
        return firstNonNull(rate, Rate.rps);
    }

    public enum Rate {
        rps, rpm, ui;

        private String caption() {
            return this == ui ? "rate" : name();
        }

        private String multiplier() {
            switch (this) {
                case rps:
                    return "1";
                case rpm:
                    return "60";
                case ui:
                    return "$" + Constants.UI_VAR_RATE_UNIT_NAME;
                default:
                    throw new IllegalStateException("Unknown rate: " + this);
            }
        }

        private boolean hasMultiplier() {
            return !"1".equals(multiplier());
        }
    }

    private SensorValueType sensor() {
        return firstNonNull(sensor, counter);
    }
}
