package ru.yandex.ci.storage.core.db.clickhouse.test_event;

import java.time.Instant;
import java.util.List;
import java.util.Set;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.storage.core.clickhouse.sp.Result;
import ru.yandex.misc.enums.IntEnum;

@Value
@AllArgsConstructor
@Builder
public class TestEventEntity {
    Instant date;
    int revision;
    int branchId;
    String author;
    String message;
    Set<StrongMode> strongModes;
    Set<Result.TestType> strongStages;
    List<String> mainJobNames;
    List<Integer> mainJobPartitions;
    List<String> disabledToolchains;
    List<String> fastTargets;
    String advisedPool;

    public enum StrongMode implements IntEnum {
        STRONG(1),
        MUTED(2),
        INTERNAL(3),
        TIMEOUT(4),
        FLAKY(5),
        YA_EXTERNAL(6);

        private final int value;

        StrongMode(int value) {
            this.value = value;
        }

        @Override
        public int value() {
            return value;
        }
    }
}
