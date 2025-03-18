package ru.yandex.ci.storage.core.db.model.check_id_generator;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.stream.Collectors;

import com.google.common.base.Preconditions;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;

@Slf4j
public class CheckIdGenerator {
    private CheckIdGenerator() {

    }

    public static List<Long> generate(Long start, int count) {
        Preconditions.checkState(start >= 0);

        var result = new ArrayList<Long>(count);

        var head = start / CheckEntity.ID_START;
        var tail = start % CheckEntity.ID_START;

        for (var i = 0; i < count; ++i) {
            head++;
            if (head > CheckEntity.NUMBER_OF_ID_PARTITIONS) {
                head = 1;
                tail++;
            }

            result.add(head * CheckEntity.ID_START + tail);
        }

        return result;
    }

    public static void fillDb(CiStorageDb db, int numberOfFreeIds) {
        var ids = db.currentOrReadOnly(() -> db.checkIds().readTable().collect(Collectors.toList()));

        if (ids.size() > numberOfFreeIds) {
            log.info("Number of free ids: {}", ids.size());
            return;
        }

        var max = ids.stream()
                .max(Comparator.comparing(x -> x.getId().getId()))
                .orElse(new CheckIdGeneratorEntity(new CheckIdGeneratorEntity.Id(1), 0L));

        log.info("Max id: {}", max);
        var newIds = CheckIdGenerator.generate(max.getValue(), numberOfFreeIds - ids.size());
        log.info("Generated {} ids", newIds.size());

        var maxId = max.getId().getId();

        var entities = new ArrayList<CheckIdGeneratorEntity>(newIds.size());
        for (var i = 0; i < newIds.size(); ++i) {
            entities.add(
                    new CheckIdGeneratorEntity(new CheckIdGeneratorEntity.Id(maxId + i + 1), newIds.get(i))
            );
        }

        db.currentOrTx(() -> db.checkIds().bulkUpsert(entities, 4086));
    }
}
