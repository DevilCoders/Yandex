package yandex.cloud.dashboard.model.spec.panel.template;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.google.common.base.Preconditions;
import lombok.Value;
import lombok.With;
import yandex.cloud.dashboard.model.spec.generic.LabelsSpec;
import yandex.cloud.dashboard.model.spec.generic.QueryParamsSpec;
import yandex.cloud.dashboard.model.spec.panel.GraphSpec;
import yandex.cloud.dashboard.model.spec.panel.QuerySpec;
import yandex.cloud.dashboard.model.spec.panel.SensorValueType;
import yandex.cloud.dashboard.model.spec.panel.YAxisInListSpec;
import yandex.cloud.dashboard.util.ObjectUtils;
import yandex.cloud.dashboard.util.StreamBuilder;

import java.util.List;
import java.util.stream.Collectors;

import static com.google.common.base.MoreObjects.firstNonNull;
import static yandex.cloud.dashboard.model.spec.Constants.FORMAT_S;
import static yandex.cloud.dashboard.model.spec.panel.SensorValueType.UseCase.HIST_BUCKET;
import static yandex.cloud.dashboard.model.spec.panel.SensorValueType.counter;
import static yandex.cloud.dashboard.util.Mergeable.mergeNullable;

/**
 * @author ssytnik
 */
@With
@Value
public class PercentileTemplate implements GraphTemplate {
    private static final List<String> DEFAULT_LEVELS = List.of("50", "75", "90", "99");

    List<String> levels;
    SensorFormat format;
    SensorValueType sensor;
    Boolean groupLines;

    @Override
    public GraphSpec transform(GraphSpec source) {
        Preconditions.checkArgument(source.getQueries().size() == 1, getClass().getSimpleName() + ": expected single query");

        List<String> effectiveLevels = firstNonNull(levels, DEFAULT_LEVELS);

        Preconditions.checkArgument(effectiveLevels.stream().allMatch(ObjectUtils::isValidDouble),
                "Invalid array of double percentile levels:", effectiveLevels);

        QuerySpec querySpec = source.getQueries().iterator().next();

        return source.toBuilder()
                .queries(effectiveLevels.stream().map(level -> query(querySpec, level)).collect(Collectors.toList()))
                .yAxes(mergeNullable(source.getYAxes(), List.of(new YAxisInListSpec(null, FORMAT_S, null, null, null, null))))
                .build();
    }

    private QuerySpec query(QuerySpec source, String level) {
        return source.toBuilder()
                .params(mergeNullable(source.getParams(), new QueryParamsSpec(formatSafe().getLabels(), null, null)))
                .groupByTime(sensor().groupByTime(HIST_BUCKET))
                .select(sensor().selectBuilder(HIST_BUCKET)
                        .addCallIf(firstNonNull(groupLines, false), "group_by_labels", "sum", formatSafe().getBucketLabel())
                        .addCall("hist", level, formatSafe().getBucketLabel())
                        .addCall("alias", "p" + level)
                        .build())
                .build();
    }

    private SensorFormat formatSafe() {
        return firstNonNull(format, SensorFormat.prometheus);
    }

    @Value
    public static class SensorFormat {
        static final SensorFormat prometheus = new SensorFormat("prometheus");

        String bucketLabel;
        LabelsSpec labels;

        @JsonCreator
        public SensorFormat(String format) {
            StreamBuilder<String> labelsBuilder = StreamBuilder.create();
            if ("prometheus".equals(format)) {
                bucketLabel = "le";
                labelsBuilder.add("hist_type=bin");
            } else if ("solomon".equals(format)) {
                bucketLabel = "bin";
            } else {
                bucketLabel = format;
            }
            labelsBuilder.add(bucketLabel + "=*");
            labels = new LabelsSpec(labelsBuilder.toString(", "));
        }
    }

    private SensorValueType sensor() {
        return firstNonNull(sensor, counter);
    }
}
