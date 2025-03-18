package ru.yandex.ci.client.sandbox;

import java.util.HashMap;
import java.util.NavigableSet;
import java.util.Set;

import javax.annotation.Nonnull;

import lombok.Builder;
import lombok.EqualsAndHashCode;
import lombok.Singular;
import lombok.Value;

import ru.yandex.ci.client.sandbox.api.SandboxTaskStatus;

@Value
@EqualsAndHashCode(callSuper = true)
@Builder(toBuilder = true)
public class TasksFilter extends AbstractFilter {

    Integer limit;

    Integer offset;

    @Singular
    @Nonnull
    NavigableSet<String> fields;

    @Singular
    @Nonnull
    NavigableSet<Long> ids;

    @Singular
    @Nonnull
    NavigableSet<SandboxTaskStatus> statuses;

    @Singular
    @Nonnull
    NavigableSet<String> types;

    @Singular
    @Nonnull
    NavigableSet<String> tags;

    @Singular
    @Nonnull
    NavigableSet<String> hints;

    TimeRange created;
    Boolean hidden;
    Boolean children;
    Boolean allTags;
    Integer semaphoreAcquirers;

    @Override
    public Set<Entry<String, Object>> entrySet() {
        var map = new HashMap<String, Object>();
        addNonNull(map, "limit", limit);
        addNonNull(map, "offset", offset);
        addNonNull(map, "created", created);
        addNonNull(map, "hidden", hidden);
        addNonNull(map, "children", children);
        addNonNull(map, "fields", fields);
        addNonNull(map, "id", ids);
        addNonNull(map, "status", statuses);
        addNonNull(map, "type", types);
        addNonNull(map, "tags", tags);
        addNonNull(map, "hints", hints);
        addNonNull(map, "all_tags", allTags);
        addNonNull(map, "semaphore_acquirers", semaphoreAcquirers);

        return map.entrySet();
    }

}
