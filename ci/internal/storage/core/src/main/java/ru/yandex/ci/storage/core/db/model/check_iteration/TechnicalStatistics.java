package ru.yandex.ci.storage.core.db.model.check_iteration;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
@Builder
@AllArgsConstructor
@Deprecated // use Metrics
public class TechnicalStatistics {
    public static final TechnicalStatistics EMPTY = new TechnicalStatistics(0, 0, 0);

    double machineHours; // actually seconds
    int totalNumberOfNodes;
    double cacheHit;

    public TechnicalStatistics plus(TechnicalStatistics other) {
        return new TechnicalStatistics(
                this.machineHours + other.machineHours,
                this.totalNumberOfNodes + other.totalNumberOfNodes,
                this.cacheHit + other.cacheHit
        );
    }

    public TechnicalStatistics minus(TechnicalStatistics other) {
        return plus(new TechnicalStatistics(-other.machineHours, -other.totalNumberOfNodes, -other.cacheHit));
    }

    public TechnicalStatistics max(TechnicalStatistics other) {
        return new TechnicalStatistics(
                Math.max(getMachineHours(), other.getMachineHours()),
                Math.max(getTotalNumberOfNodes(), other.getTotalNumberOfNodes()),
                Math.max(getCacheHit(), other.getCacheHit())
        );
    }
}
