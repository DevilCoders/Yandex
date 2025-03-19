package yandex.cloud.dashboard.model.spec.panel.template;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.google.common.base.Preconditions;
import com.google.common.collect.Iterables;
import lombok.AllArgsConstructor;
import lombok.Value;
import lombok.With;
import yandex.cloud.dashboard.model.spec.Spec;
import yandex.cloud.dashboard.model.spec.panel.FunctionParamsSpec;
import yandex.cloud.dashboard.model.spec.panel.GraphSpec;
import yandex.cloud.dashboard.model.spec.panel.QuerySpec;
import yandex.cloud.dashboard.model.spec.validator.SpecValidationContext;

import java.util.LinkedHashMap;
import java.util.Objects;

import static com.google.common.base.MoreObjects.firstNonNull;
import static java.util.stream.Collectors.toList;
import static yandex.cloud.dashboard.model.spec.panel.QuerySpec.SelectBuilder.selectBuilder;
import static yandex.cloud.dashboard.util.ObjectUtils.takeOrDefault;
import static yandex.cloud.util.Predicates.oneOf;

/**
 * @author ssytnik
 */
@With
@Value
public class PatchSelectTemplate implements GraphTemplate {
    ElementSpec before;
    ElementSpec after;
    LinkedHashMap<String, FunctionParamsSpec> add;

    @Override
    public void validate(SpecValidationContext context) {
        Preconditions.checkArgument(oneOf(before, after), "Either 'before' or 'after' should be specified");
        Preconditions.checkNotNull(add, "'add' should be specified");
    }

    @Override
    public GraphSpec transform(GraphSpec source) {
        return source.toBuilder()
                .queries(source.getQueries().stream().map(this::query).collect(toList()))
                .build();
    }

    private QuerySpec query(QuerySpec source) {
        return source.toBuilder()
                .select(patch(source.getSelect()))
                .build();
    }

    public LinkedHashMap<String, FunctionParamsSpec> patch(LinkedHashMap<String, FunctionParamsSpec> select0) {
        LinkedHashMap<String, FunctionParamsSpec> select = takeOrDefault(select0, LinkedHashMap::new);
        int index = firstNonNull(before, after).resolveIndex(select) + (before != null ? -1 : 0);
        QuerySpec.SelectBuilder b = selectBuilder();
        select.entrySet().stream().limit(index).forEach(b::addCall);
        b.addCalls(add);
        select.entrySet().stream().skip(index).forEach(b::addCall);
        return b.build();
    }

    @With
    @Value
    @AllArgsConstructor
    public static class ElementSpec implements Spec {
        String name; // first, last, or function name
        Integer index; // 1-based index == 1 .. N

        @JsonCreator
        public ElementSpec(String name) {
            this(name, null);
        }

        @JsonCreator
        public ElementSpec(Integer index) {
            this(null, index);
        }

        @Override
        public void validate(SpecValidationContext context) {
            Preconditions.checkArgument(oneOf(name, index), "Either 'name' or 'index' should be specified");
            Preconditions.checkArgument(index == null || index >= 1, "'index' should be positive");
        }

        public int resolveIndex(LinkedHashMap<String, FunctionParamsSpec> select) {
            if (index != null) {
                Preconditions.checkArgument(index <= select.size(), "'index' is too large: %s", index);
                return index;
            } else if ("first".equals(name)) {
                return 1;
            } else if ("last".equals(name)) {
                return select.size();
            } else {
                int index = Iterables.indexOf(select.entrySet(), e -> Objects.requireNonNull(e).getKey().equals(name));
                Preconditions.checkArgument(index > -1, "Function '%s' not found in query select", name);
                return 1 + index;
            }
        }
    }
}
