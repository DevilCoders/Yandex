package ru.yandex.ci.storage.core.message;

@Deprecated // https://st.yandex-team.ru/CI-3995
public class PartitionGeneratorBase {
    private final int numberOfPartitions;

    public PartitionGeneratorBase(int numberOfPartitions) {
        this.numberOfPartitions = numberOfPartitions;
    }

    /**
     * Generates partition number from check id hash code
     * Note: .hashCode() could return negative values, to avoid this, use bit mask 0xfffffff
     */
    protected int generatePartitionInternal(int checkIdHashCode) {
        return ((checkIdHashCode & 0xfffffff) % numberOfPartitions);
    }
}
