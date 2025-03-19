package yandex.cloud.dashboard.model.result.panel;

import com.google.common.base.Preconditions;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.Streams;
import lombok.Value;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import static yandex.cloud.dashboard.model.result.panel.FunctionDescriptor.NormalCallDescriptor.Quoting.AS_IS;
import static yandex.cloud.dashboard.model.result.panel.FunctionDescriptor.NormalCallDescriptor.Quoting.CHAINED;
import static yandex.cloud.dashboard.model.result.panel.FunctionDescriptor.NormalCallDescriptor.Quoting.QUOTED;
import static yandex.cloud.dashboard.model.result.panel.FunctionDescriptor.NormalCallDescriptor.normalCall;

/**
 * @author ssytnik
 */
@Value(staticConstructor = "of")
public class FunctionDescriptor {
    private static final Map<String, FunctionDescriptor> descriptors;

    boolean addToSelect;
    String name;
    CallDescriptor callDescriptor;

    static {
        Map<String, FunctionDescriptor> m = new HashMap<>();

        put(m, of(true, "alias", normalCall(CHAINED, QUOTED)));
        put(m, of(true, "asap", normalCall(CHAINED)), "asap");
        put(m, of(true, "bottom", normalCall(AS_IS, QUOTED, CHAINED)));
//        put(m, of(true, "bottom_sum", normalCall(AS_IS, CHAINED)));
//        put(m, of(true, "bottom_max", normalCall(AS_IS, CHAINED)));
//        put(m, of(true, "bottom_min", normalCall(AS_IS, CHAINED)));
//        put(m, of(true, "bottom_avg", normalCall(AS_IS, CHAINED)));
//        put(m, of(true, "bottom_count", normalCall(AS_IS, CHAINED)));
        put(m, of(true, "derivative", normalCall(CHAINED)), "deriv");
        put(m, of(true, "diff", normalCall(CHAINED)));
        put(m, of(true, "drop_above", normalCall(CHAINED, AS_IS)), "le");
        put(m, of(true, "drop_below", normalCall(CHAINED, AS_IS)), "ge");
        put(m, of(true, "drop_nan", normalCall(CHAINED)));
        put(m, of(true, "group_by_time", normalCall(AS_IS, QUOTED, CHAINED)));
        put(m, of(true, "group_by_labels", new GroupByLabelsCallDescriptor()));
        put(m, of(true, "group_lines", normalCall(QUOTED, CHAINED)));
        put(m, of(true, "histogram_percentile", normalCall(AS_IS, QUOTED, CHAINED)), "hist");
        put(m, of(true, "integrate_fn", normalCall(CHAINED)));
        put(m, of(true, "math", new MathCallDescriptor()));
        put(m, of(true, "moving_avg", normalCall(CHAINED, AS_IS)));
        put(m, of(true, "non_negative_derivative", normalCall(CHAINED)), "nn_deriv");
        put(m, of(true, "percentile_group_lines", normalCall(AS_IS, CHAINED)));
        put(m, of(true, "replace_nan", normalCall(CHAINED, AS_IS)));
        put(m, of(true, "shift", normalCall(CHAINED, AS_IS)));
        put(m, of(true, "top", normalCall(AS_IS, QUOTED, CHAINED)));
//        put(m, of(true, "top_sum", normalCall(AS_IS, CHAINED)));
//        put(m, of(true, "top_max", normalCall(AS_IS, CHAINED)));
//        put(m, of(true, "top_min", normalCall(AS_IS, CHAINED)));
//        put(m, of(true, "top_avg", normalCall(AS_IS, CHAINED)));
//        put(m, of(true, "top_count", normalCall(AS_IS, CHAINED)));

        descriptors = Map.copyOf(m);
    }

    private static void put(Map<String, FunctionDescriptor> map, FunctionDescriptor fd, String... aliases) {
        map.put(fd.getName(), fd);
        Stream.of(aliases).forEach(alias -> map.put(alias, fd));
    }

    static FunctionDescriptor get(String fn) {
        Preconditions.checkArgument(descriptors.containsKey(fn), "Unknown function name or alias: '%s'", fn);
        return descriptors.get(fn);
    }

    String wrapWithCall(String chainedResult, List<String> otherParameters) {
        return callDescriptor.wrap(chainedResult, name, otherParameters);
    }


    private interface CallDescriptor {
        String wrap(String chainedResult, String name, List<String> otherParameters);
    }

    @Value
    public static class NormalCallDescriptor implements CallDescriptor {
        List<Quoting> parameterQuoting;

        static NormalCallDescriptor normalCall(Quoting... parameterQuoting) {
            return new NormalCallDescriptor(List.of(parameterQuoting));
        }

        @SuppressWarnings("UnstableApiUsage")
        @Override
        public String wrap(String chainedResult, String name, List<String> otherParameters) {
            Preconditions.checkArgument(parameterCount() - 1 == otherParameters.size(),
                    "Expected %s parameter(s) but got %s for '%s': %s",
                    parameterCount() - 1, otherParameters.size(), name, otherParameters);

            List<String> parameters = ImmutableList.<String>builder()
                    .addAll(otherParameters.subList(0, chainedResultIndex()))
                    .add(chainedResult)
                    .addAll(otherParameters.subList(chainedResultIndex(), otherParameters.size()))
                    .build();

            String serializedParameters = Streams.mapWithIndex(parameters.stream(), this::quoted)
                    .collect(Collectors.joining(", "));

            return String.format("%s(%s)", name, serializedParameters);
        }

        // TODO cache (with @Lazy?)
        private int chainedResultIndex() {
            return parameterQuoting.indexOf(CHAINED);
        }

        private String quoted(String parameter, long index) {
            return parameterQuoting.get((int) index) == QUOTED ? "'" + parameter + "'" : parameter;
        }

        private int parameterCount() {
            return parameterQuoting.size();
        }

        enum Quoting {
            CHAINED, QUOTED, AS_IS
        }
    }

    @Value
    public static class GroupByLabelsCallDescriptor implements CallDescriptor {
        @Override
        public String wrap(String chainedResult, String name, List<String> otherParameters) {
            Preconditions.checkArgument(otherParameters.size() >= 2,
                    "Expected '%s' params to be: <fn>, label [, label...], but got %s", name, otherParameters);

            return String.format("%s(%s) by (%s)",
                    otherParameters.get(0), chainedResult, String.join(", ", otherParameters.subList(1, otherParameters.size())));
        }
    }

    @Value
    public static class MathCallDescriptor implements CallDescriptor {
        @Override
        public String wrap(String chainedResult, String name, List<String> otherParameters) {
            Preconditions.checkArgument(otherParameters.size() == 1,
                    "Expected one math operation like '/ 100', but got %s", otherParameters);

            return String.format("%s %s", chainedResult, otherParameters.get(0));
        }
    }

}
