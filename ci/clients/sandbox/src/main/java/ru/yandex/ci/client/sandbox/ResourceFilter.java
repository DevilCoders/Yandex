package ru.yandex.ci.client.sandbox;

import java.util.HashMap;
import java.util.List;
import java.util.Set;

import javax.annotation.Nonnull;

import lombok.Builder;
import lombok.EqualsAndHashCode;
import lombok.Singular;
import lombok.Value;

@Value
@EqualsAndHashCode(callSuper = true)
@Builder(toBuilder = true)
public class ResourceFilter extends AbstractFilter {
    @Singular
    @Nonnull
    List<Long> taskIds;

    String type;

    @Override
    public Set<Entry<String, Object>> entrySet() {
        var map = new HashMap<String, Object>();
        addNonNull(map, "task_id", taskIds);
        addNonNull(map, "type", type);
        return map.entrySet();
    }

}
