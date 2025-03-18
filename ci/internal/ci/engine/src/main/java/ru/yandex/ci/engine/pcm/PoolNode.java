package ru.yandex.ci.engine.pcm;

import java.time.Instant;
import java.util.Comparator;

import javax.annotation.Nonnull;

import lombok.AllArgsConstructor;
import lombok.Value;

import ru.yandex.ci.core.db.autocheck.model.AccessControl;
import ru.yandex.ci.core.db.autocheck.model.PoolNodeEntity;

@Value
@AllArgsConstructor
public class PoolNode {
    @Nonnull
    AccessControl acId;
    @Nonnull
    String poolPath;
    int resourceGuarantiesSlots;
    double weight;

    public PoolNode(AccessControl acId, PoolNodeEntity poolNodeEntity) {
        this(acId, poolNodeEntity.getId().getName(),
                poolNodeEntity.getResourceGuarantiesSlots(), poolNodeEntity.getWeight());
    }

    public PoolNodeEntity toPoolNodeEntity(Instant updated) {
        return PoolNodeEntity.builder()
                .id(new PoolNodeEntity.Id(updated, getPoolPath()))
                .resourceGuarantiesSlots(getResourceGuarantiesSlots())
                .weight(getWeight())
                .build();
    }

    /**
     * Compares by ResourceGuarantiesSlots, Weight, PoolPath
     */
    public static int compare(PoolNode node1, PoolNode node2) {
        return Comparator.comparingInt(PoolNode::getResourceGuarantiesSlots)
                .thenComparingDouble(PoolNode::getWeight)
                .thenComparing(PoolNode::getPoolPath).compare(node1, node2);
    }
}
