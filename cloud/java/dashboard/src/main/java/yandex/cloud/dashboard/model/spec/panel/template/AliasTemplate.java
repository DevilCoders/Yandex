package yandex.cloud.dashboard.model.spec.panel.template;

import com.google.common.base.Preconditions;
import lombok.Value;
import lombok.With;
import yandex.cloud.dashboard.model.spec.panel.DrawSpec;
import yandex.cloud.dashboard.model.spec.panel.GraphSpec;
import yandex.cloud.dashboard.model.spec.panel.QuerySpec;
import yandex.cloud.dashboard.model.spec.panel.QuerySpec.SelectBuilder;

import java.util.List;

import static com.google.common.collect.Streams.zip;
import static java.util.stream.Collectors.toList;

/**
 * Appends alias function call to query (queries):
 * <ul>
 * <li>if <code>alias == null</code>, then aliases from 'draw' clause are used, exactly 1 alias for 1 query;</li>
 * <li>otherwise, <code>alias</code> explicitly specifies an alias for the single query without wildcards in the graph.</li>
 * </ul>
 *
 * @author ssytnik
 */
@With
@Value
public class AliasTemplate implements GraphTemplate {
    public static final AliasTemplate ALIASES_FROM_DRAW = new AliasTemplate(null);

    String alias;

    @Override
    public GraphSpec transform(GraphSpec source) {
        List<QuerySpec> queries = source.getQueries();
        List<String> draw;

        if (alias == null) {
            Preconditions.checkArgument(source.getDraw() != null,
                    "Without 'alias' template parameter, 'draw' should be specified");
            draw = source.getDraw().stream().map(DrawSpec::getAlias).collect(toList());
        } else {
            // TODO do our best to make sure that query labels address a single line on the graph
            draw = List.of(alias);
        }

        Preconditions.checkArgument(queries.size() == draw.size(),
                "Queries size (%s) should match inferred 'draw' list size (%s) at '%s', but draw = %s",
                queries.size(), draw.size(), getClass().getSimpleName(), draw);

        return source
                .withQueries(zip(queries.stream(), draw.stream(), this::query).collect(toList()));
    }

    private QuerySpec query(QuerySpec source, String alias) {
        return source.withSelect(
                SelectBuilder.selectBuilder(source.getSelect())
                        .addCall("alias", alias)
                        .build());
    }
}
