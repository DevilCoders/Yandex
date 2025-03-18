package ru.yandex.ci.storage.core.message;

import ru.yandex.ci.storage.core.db.model.check.CheckEntity;

@Deprecated // https://st.yandex-team.ru/CI-3995
public class InternalPartitionGenerator extends PartitionGeneratorBase {
    public InternalPartitionGenerator(int numberOfPartitions) {
        super(numberOfPartitions);
    }

    /**
     * Generates partition number from check id hash code
     */
    public int generatePartition(CheckEntity.Id checkId) {
        return generatePartitionInternal(checkId.hashCode());
    }
}
