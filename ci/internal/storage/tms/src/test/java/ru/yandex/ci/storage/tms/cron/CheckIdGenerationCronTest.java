package ru.yandex.ci.storage.tms.cron;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.storage.core.StorageYdbTestBase;
import ru.yandex.ci.storage.core.db.model.check_id_generator.CheckIdGeneratorEntity;

import static org.assertj.core.api.Assertions.assertThat;

public class CheckIdGenerationCronTest extends StorageYdbTestBase {
    @Test
    public void generates() {
        var task = new CheckIdGenerationCron(db, 8, 1);
        task.execute(null);

        var ids = this.db.currentOrTx(() -> this.db.checkIds().findAll());

        assertThat(ids).hasSize(8);

        task.execute(null);

        ids = this.db.currentOrTx(() -> this.db.checkIds().findAll());
        assertThat(ids).hasSize(8);

        this.db.currentOrTx(() -> this.db.checkIds().delete(new CheckIdGeneratorEntity.Id(2)));
        ids = this.db.currentOrTx(() -> this.db.checkIds().findAll());
        assertThat(ids).hasSize(7);

        task.execute(null);

        ids = this.db.currentOrTx(() -> this.db.checkIds().findAll());
        assertThat(ids).hasSize(8);
    }
}
