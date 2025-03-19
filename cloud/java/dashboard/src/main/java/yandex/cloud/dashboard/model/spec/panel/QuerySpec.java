package yandex.cloud.dashboard.model.spec.panel;

import com.google.common.base.Preconditions;
import lombok.Builder;
import lombok.Value;
import lombok.With;
import yandex.cloud.dashboard.model.spec.Spec;
import yandex.cloud.dashboard.model.spec.generic.QueryParamsSpec;
import yandex.cloud.dashboard.model.spec.validator.SpecValidationContext;

import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * @author ssytnik
 */
@Builder(toBuilder = true)
@With
@Value
public class QuerySpec implements Spec {
    String expr;

    QueryParamsSpec params;
    GroupByTimeSpec groupByTime;
    // TODO maybe list (OR list of __list__), verbose but allows multiple calls of same function
    LinkedHashMap<String, FunctionParamsSpec> select;

    @Override
    public void validate(SpecValidationContext context) {
        if (expr != null) {
            Preconditions.checkArgument(groupByTime == null && select == null,
                    "Cannot specify 'groupByTime' and 'select' when raw expression is specified");
        }
    }

    @SuppressWarnings("UnusedReturnValue")
    public static class SelectBuilder {
        LinkedHashMap<String, FunctionParamsSpec> map = new LinkedHashMap<>();

        private SelectBuilder() {
        }

        public static SelectBuilder selectBuilder() {
            return new SelectBuilder();
        }

        public static SelectBuilder selectBuilder(Map<String, FunctionParamsSpec> src) {
            return selectBuilder().addCalls(src);
        }

        public SelectBuilder addCalls(Map<String, FunctionParamsSpec> src) {
            map.putAll(src);
            return this;
        }

        public SelectBuilder addCall(Map.Entry<String, FunctionParamsSpec> entry) {
            map.put(entry.getKey(), entry.getValue());
            return this;
        }

        public SelectBuilder addCall(String fn, List<String> params) {
            map.put(fn, new FunctionParamsSpec(params));
            return this;
        }

        public SelectBuilder addCall(String fn, String... params) {
            return addCall(fn, List.of(params));
        }

        public SelectBuilder addCallIf(Boolean condition, String fn, List<String> params) {
            if (condition != null && condition) {
                return addCall(fn, params);
            } else {
                return this;
            }
        }

        public SelectBuilder addCallIf(Boolean condition, String fn, String... params) {
            return addCallIf(condition, fn, List.of(params));
        }

        public LinkedHashMap<String, FunctionParamsSpec> build() {
            return map;
        }
    }
}
