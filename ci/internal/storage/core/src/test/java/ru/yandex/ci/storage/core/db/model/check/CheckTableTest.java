package ru.yandex.ci.storage.core.db.model.check;

import java.util.List;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.common.ydb.YdbExecutor;
import ru.yandex.ci.storage.core.StorageYdbTestBase;
import ru.yandex.ci.storage.core.db.CiStorageDbTables;
import ru.yandex.ci.test.TestUtils;

import static org.assertj.core.api.Assertions.assertThat;

class CheckTableTest extends StorageYdbTestBase {

    @Autowired
    private YdbExecutor executor;

    @Test
    void backwardCompatibility() {
        TestUtils.forTable(db, CiStorageDbTables::checks)
                .upsertValues(executor, "ydb-data/checks.json");
        var items = db.currentOrReadOnly(() -> db.checks().findAll());
        assertThat(items).isNotEmpty();
        assertThat(items)
                .extracting(CheckEntity::getAutostartLargeTests)
                .isEqualTo(List.of(
                        List.of(new LargeAutostart(null, "ci/demo-project/large-tests", List.of()))
                ));
    }
}
