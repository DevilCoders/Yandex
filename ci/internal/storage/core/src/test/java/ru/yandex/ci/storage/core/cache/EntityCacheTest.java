package ru.yandex.ci.storage.core.cache;

import java.time.Instant;
import java.util.Set;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.storage.core.StorageYdbTestBase;
import ru.yandex.ci.storage.core.cache.impl.ChecksCacheImpl;
import ru.yandex.ci.storage.core.cache.impl.StaleModificationException;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.common.StorageRevision;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;

@SuppressWarnings("OptionalGetWithoutIsPresent")
public class EntityCacheTest extends StorageYdbTestBase {

    @Test
    public void gets() {
        var id = CheckEntity.Id.of("1");
        db.currentOrTx(() -> this.db.checks().save(createCheck(id)));

        var cache = new ChecksCacheImpl(db, 100, meterRegistry);
        var cached = cache.get(id);
        assertThat(cached).isNotNull();
        assertThat(cached.isEmpty()).isFalse();
        assertThat(cached.get().getId()).isEqualTo(id);
    }

    @Test
    public void getFromModifiable() {
        var id = CheckEntity.Id.of("1");
        db.currentOrTx(() -> this.db.checks().save(createCheck(id)));

        var cache = new ChecksCacheImpl(db, 100, meterRegistry).toModifiable(1);

        var cached = cache.get(id);
        assertThat(cached).isNotNull();
        assertThat(cached.isEmpty()).isFalse();
        assertThat(cached.get().getId()).isEqualTo(id);
    }

    @Test
    public void getFreshFetchesFromDbOnce() {
        var id = CheckEntity.Id.of("1");
        var secondId = CheckEntity.Id.of("2");
        var thirdId = CheckEntity.Id.of("3");
        db.currentOrTx(() -> this.db.checks().save(createCheck(id)));
        db.currentOrTx(() -> this.db.checks().save(createCheck(thirdId)));

        var cache = new ChecksCacheImpl(db, 100, meterRegistry).toModifiable(2);

        var cached = cache.getFresh(Set.of(id, secondId));
        assertThat(cached).hasSize(1);
        db.currentOrTx(() -> this.db.checks().save(createCheck(secondId)));
        cached = cache.getFresh(Set.of(id, secondId));
        assertThat(cached).withFailMessage("Already loaded second id must not be loaded").hasSize(1);
        assertThat(cache.getFresh(secondId)).isEmpty();

        cached = cache.getFresh(Set.of(secondId, thirdId));
        assertThat(cached).withFailMessage("Not loaded third id must present").hasSize(1);
        assertThat(cached.get(0).getId()).isEqualTo(thirdId);
    }

    @Test
    public void forbidsWriteWithoutRead() {
        var id = CheckEntity.Id.of(1L);
        var cache = new ChecksCacheImpl(db, 100, meterRegistry);
        var modifiable = cache.toModifiable(2);
        modifiable.getFresh(id);
        modifiable.put(createCheck(id));

        var secondModifiable = cache.toModifiable(1);
        assertThatThrownBy(
                () -> secondModifiable.put(createCheck(id))
        ).isInstanceOf(StaleModificationException.class);
    }

    private CheckEntity createCheck(CheckEntity.Id id) {
        return CheckEntity.builder()
                .id(id)
                .left(StorageRevision.EMPTY)
                .right(StorageRevision.EMPTY)
                .build();
    }

    @Test
    public void updates() {
        var id = CheckEntity.Id.of("1");
        var check = CheckEntity.builder()
                .id(id)
                .left(new StorageRevision("l1", "", 1, Instant.EPOCH))
                .right(StorageRevision.EMPTY)
                .build();

        var changed = check.toBuilder()
                .left(new StorageRevision("l2", "", 1, Instant.EPOCH))
                .right(StorageRevision.EMPTY)
                .build();

        db.currentOrTx(() -> this.db.checks().save(check));

        var cache = new ChecksCacheImpl(db, 100, meterRegistry);
        var modifiable = cache.toModifiable(2);
        modifiable.getFresh(id);
        modifiable.put(changed);

        assertThat(cache.get(id).get().getLeft().getBranch()).isEqualTo("l1");
        assertThat(modifiable.get(id).get().getLeft().getBranch()).isEqualTo("l2");
        modifiable.commit();
        assertThat(cache.get(id).get().getLeft().getBranch()).isEqualTo("l2");
    }

    @Test
    public void invalidates() {
        var id = CheckEntity.Id.of("1");
        var check = CheckEntity.builder().id(id)
                .left(new StorageRevision("l1", "", 1, Instant.EPOCH))
                .right(StorageRevision.EMPTY)
                .build();

        var changed = check.toBuilder()
                .left(new StorageRevision("l2", "", 1, Instant.EPOCH))
                .build();

        db.currentOrTx(() -> this.db.checks().save(check));

        var cache = new ChecksCacheImpl(db, 100, meterRegistry);
        assertThat(cache.get(id).get().getLeft().getBranch()).isEqualTo("l1");

        var modifiable = cache.toModifiable(1);
        db.currentOrTx(() -> this.db.checks().save(changed));
        modifiable.invalidate(id);

        assertThat(cache.get(id).get().getLeft().getBranch()).isEqualTo("l1");
        modifiable.commit();
        assertThat(cache.get(id).get().getLeft().getBranch()).isEqualTo("l2");
    }
}
