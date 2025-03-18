package ru.yandex.ci.observer.reader.message;

import ru.yandex.ci.observer.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.message.PartitionGeneratorBase;

public class InternalPartitionGenerator extends PartitionGeneratorBase {
    public InternalPartitionGenerator(int numberOfPartitions) {
        super(numberOfPartitions);
    }

    /**
     * Generates partition number from check id hash code
     * Note: .hashCode() could return negative values, to avoid this, use bit mask 0xfffffff
     */
    public int generatePartition(CheckEntity.Id checkId) {
        return generatePartitionInternal(checkId.hashCode());
    }
}
