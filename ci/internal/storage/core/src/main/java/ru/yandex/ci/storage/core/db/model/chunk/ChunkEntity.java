package ru.yandex.ci.storage.core.db.model.chunk;

import lombok.Builder;
import lombok.Value;
import lombok.With;

import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.inside.yt.kosher.impl.ytree.object.annotation.YTreeFlattenField;

@SuppressWarnings({"ReferenceEquality", "BoxedPrimitiveEquality"})
@Value
@Builder(toBuilder = true)
@With
@Table(name = "Chunks")
public class ChunkEntity implements Entity<ChunkEntity> {
    @YTreeFlattenField
    ChunkEntity.Id id;

    Common.ChunkStatus status;

    int partition;

    @Override
    public ChunkEntity.Id getId() {
        return id;
    }

    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<ChunkEntity> {
        Common.ChunkType chunkType;
        Integer number;

        @Override
        public String toString() {
            return this.getName(chunkType) + number.toString();
        }

        private String getName(Common.ChunkType chunkType) {
            return switch (chunkType) {
                case CT_CONFIGURE -> "C";
                case CT_BUILD -> "B";
                case CT_STYLE -> "Y";
                case CT_SMALL_TEST -> "S";
                case CT_MEDIUM_TEST -> "M";
                case CT_LARGE_TEST -> "L";
                case CT_TESTENV -> "T";
                case CT_NATIVE_BUILD -> "N";
                case UNRECOGNIZED -> throw new RuntimeException();
            };
        }
    }
}
