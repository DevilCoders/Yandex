package ru.yandex.ci.storage.core.db.model.test;

import com.google.common.primitives.UnsignedLongs;
import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;

import ru.yandex.ci.storage.core.TaskMessages;

// Imaginary entity, not exists in db.
@Value
@Table(name = "Test [imaginary]")
public class TestEntity implements Entity<TestEntity> {
    public static final String ALL_TOOLCHAINS = "@all";

    Id id;

    @Override
    public Entity.Id<TestEntity> getId() {
        return id;
    }

    @Value
    public static class Id implements Entity.Id<TestEntity> {
        @Column(dbType = DbType.UINT64)
        long suiteId;

        String toolchain;

        @Column(dbType = DbType.UINT64)
        long id;

        public static Id parse(String suiteId, String toolchain, String id) {
            var parsedSuiteId = UnsignedLongs.decode(suiteId);

            return new Id(
                    parsedSuiteId,
                    toolchain,
                    id.isEmpty()
                            ? parsedSuiteId
                            : UnsignedLongs.decode(id)
            );
        }

        public static Id of(long suiteId, String toolchain) {
            return new Id(suiteId, toolchain, suiteId);
        }

        public static Id of(TaskMessages.AutocheckTestId id) {
            return new Id(id.getSuiteHid(), id.getToolchain(), id.getHid());
        }

        public boolean isSuite() {
            return this.id == this.suiteId;
        }

        public Id toSuiteId() {
            return of(this.suiteId, this.toolchain);
        }

        public String getSuiteIdString() {
            return UnsignedLongs.toString(suiteId);
        }

        public String getIdString() {
            return UnsignedLongs.toString(id);
        }

        @SuppressWarnings("UnstableApiUsage")
        public static int getPostProcessorPartition(long hid, int numberOfPartitions) {
            return (int) UnsignedLongs.remainder(hid, numberOfPartitions);
        }

        @Override
        public String toString() {
            return "[%s/%s/%s]".formatted(
                    UnsignedLongs.toString(suiteId), toolchain, UnsignedLongs.toString(id)
            );
        }


        public Id toAllToolchainsId() {
            return new Id(this.suiteId, ALL_TOOLCHAINS, this.id);
        }

        public boolean isAggregate() {
            return toolchain.equals(ALL_TOOLCHAINS);
        }

        public Id toAutocheckChunkId(long autocheckChunkId) {
            return new Id(this.suiteId, this.toolchain, autocheckChunkId);
        }
    }
}
