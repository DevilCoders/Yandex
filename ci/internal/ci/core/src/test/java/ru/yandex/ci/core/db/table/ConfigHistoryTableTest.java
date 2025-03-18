package ru.yandex.ci.core.db.table;

import java.nio.file.Path;
import java.time.Instant;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.TreeMap;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.CommonYdbTestBase;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.ConfigCreationInfo;
import ru.yandex.ci.core.config.ConfigEntity;
import ru.yandex.ci.core.config.ConfigProblem;
import ru.yandex.ci.core.config.ConfigSecurityState;
import ru.yandex.ci.core.config.ConfigStatus;
import ru.yandex.ci.core.test.TestData;

import static org.assertj.core.api.Assertions.assertThat;

class ConfigHistoryTableTest extends CommonYdbTestBase {

    @Test
    void testCompatibility() {
        ConfigEntity entity = new ConfigEntity(
                TestData.CONFIG_PATH_ABD,
                TestData.TRUNK_R4,
                ConfigStatus.READY,
                List.of(new ConfigProblem(ConfigProblem.Level.WARN, "Ничего страшного", "Реально не обращай внимание")),
                new TreeMap<>(Map.of(TestData.TASK_ID, TestData.REVISION)),
                TestData.VALID_SECURITY_STATE,
                new ConfigCreationInfo(Instant.now(), null, null, null),
                null,
                null
        );

        db.currentOrTx(() ->
                db.configHistory().save(entity));

        assertThat(
                db.currentOrTx(() -> db.configHistory().findById(TestData.CONFIG_PATH_ABD, TestData.TRUNK_R4))
        ).contains(entity);

        assertThat(
                db.currentOrTx(() -> db.configHistory().findLastConfig(TestData.CONFIG_PATH_ABD, ArcBranch.trunk()))
        ).contains(entity);

    }

    @Test
    void testCompatibility2() {

        ConfigEntity entity = new ConfigEntity(
                TestData.CONFIG_PATH_ABD,
                TestData.TRUNK_R4,
                ConfigStatus.READY,
                List.of(new ConfigProblem(ConfigProblem.Level.WARN, "Ничего страшного", "Реально не обращай внимание")),
                new TreeMap<>(Map.of(TestData.TASK_ID, TestData.REVISION)),
                TestData.VALID_SECURITY_STATE,
                new ConfigCreationInfo(Instant.now(), null, null, null),
                null,
                null
        );

        db.currentOrTx(() -> db.configHistory().save(entity));

        var loaded = db.currentOrReadOnly(() ->
                db.configHistory().getById(TestData.CONFIG_PATH_ABD, TestData.TRUNK_R4));

        assertThat(loaded).isEqualTo(entity);

        var lastLoaded = db.currentOrReadOnly(() ->
                db.configHistory().findLastConfig(TestData.CONFIG_PATH_ABD, ArcBranch.trunk()));
        assertThat(lastLoaded).contains(entity);

    }

    @Test
    void testUpsert() {

        ConfigEntity entity = new ConfigEntity(
                TestData.CONFIG_PATH_ABD,
                TestData.TRUNK_R4,
                ConfigStatus.READY,
                List.of(new ConfigProblem(ConfigProblem.Level.WARN, "Ничего страшного", "Реально не обращай внимание")),
                new TreeMap<>(Map.of(TestData.TASK_ID, TestData.REVISION)),
                TestData.VALID_SECURITY_STATE,
                new ConfigCreationInfo(Instant.now(), null, null, null),
                null,
                null
        );

        db.currentOrTx(() -> db.configHistory().save(entity));

        assertThat(
                db.currentOrTx(() -> db.configHistory().findById(TestData.CONFIG_PATH_ABD, TestData.TRUNK_R4))
        ).contains(entity);

        assertThat(
                db.currentOrTx(() -> db.configHistory().findLastConfig(TestData.CONFIG_PATH_ABD, ArcBranch.trunk()))
        ).contains(entity);

        assertThat(
                db.currentOrTx(() -> db.configHistory().findById(TestData.CONFIG_PATH_ABD, TestData.TRUNK_R3))
        ).isEmpty();

        assertThat(
                db.currentOrTx(() -> db.configHistory().findLastConfig(TestData.CONFIG_PATH_ABD, TestData.TRUNK_R3))
        ).isEmpty();
    }

    @Test
    void testLastValidConfig() {
        ConfigEntity entity1 = new ConfigEntity(
                TestData.CONFIG_PATH_ABD,
                TestData.TRUNK_R4,
                ConfigStatus.SECURITY_PROBLEM,
                List.of(ConfigProblem.warn("Ничего страшного", "Реально не обращай внимание")),
                new TreeMap<>(Map.of(TestData.TASK_ID, TestData.REVISION)),
                TestData.NO_TOKEN_SECURITY_STATE,
                new ConfigCreationInfo(Instant.now(), null, null, null),
                null,
                "username"
        );

        ConfigEntity entity2 = new ConfigEntity(
                TestData.CONFIG_PATH_ABD,
                TestData.TRUNK_R5,
                ConfigStatus.INVALID,
                List.of(ConfigProblem.crit("Шеф, всё пропало!", "Гипс снимают, клиент уезжает!")),
                new TreeMap<>(Map.of(TestData.TASK_ID, TestData.REVISION)),
                new ConfigSecurityState(null, ConfigSecurityState.ValidationStatus.INVALID_CONFIG),
                new ConfigCreationInfo(Instant.now(), null, null, null),
                null,
                null
        );

        db.currentOrTx(
                () -> {
                    db.configHistory().save(entity1);
                    db.configHistory().save(entity2);
                }
        );

        assertThat(db.currentOrTx(() ->
                db.configHistory().findLastValidConfig(TestData.CONFIG_PATH_ABD, ArcBranch.trunk())))
                .contains(entity1);

    }

    @Test
    void testGetConfig() {

        ConfigEntity entity = new ConfigEntity(
                TestData.CONFIG_PATH_ABE,
                TestData.TRUNK_R3,
                ConfigStatus.READY,
                List.of(new ConfigProblem(ConfigProblem.Level.WARN, "Ничего страшного", "Реально не обращай внимание")),
                new TreeMap<>(Map.of(TestData.TASK_ID, TestData.REVISION)),
                TestData.VALID_SECURITY_STATE,
                new ConfigCreationInfo(Instant.now(), null, null, null),
                null,
                null
        );

        upsert(entity);


        assertThat(findConfig(TestData.CONFIG_PATH_ABE, TestData.TRUNK_R4))
                .isEmpty();

        assertThat(findLastConfig(TestData.CONFIG_PATH_ABE, TestData.TRUNK_R3.getBranch()))
                .contains(entity);

    }

    @Test
    void testGetLastConfig() {

        assertThat(findLastConfig(TestData.CONFIG_PATH_ABC, ArcBranch.trunk()))
                .isEmpty();

        ConfigEntity entity1 = new ConfigEntity(
                TestData.CONFIG_PATH_ABC,
                TestData.TRUNK_R4,
                ConfigStatus.INVALID,
                List.of(new ConfigProblem(ConfigProblem.Level.CRIT, "Ахтунг", "Не ну что за жесть")),
                new TreeMap<>(Map.of(TestData.TASK_ID, TestData.REVISION)),
                TestData.NO_TOKEN_SECURITY_STATE,
                new ConfigCreationInfo(
                        Instant.now(),
                        TestData.TRUNK_R3,
                        TestData.TRUNK_R3,
                        null
                ),
                null,
                null
        );

        ConfigEntity entity2 = new ConfigEntity(
                TestData.CONFIG_PATH_ABC,
                TestData.TRUNK_R3,
                ConfigStatus.READY,
                List.of(new ConfigProblem(ConfigProblem.Level.WARN, "Ничего страшного", "Реально не обращай внимание")),
                new TreeMap<>(Map.of(TestData.TASK_ID, TestData.REVISION)),
                TestData.VALID_SECURITY_STATE,
                new ConfigCreationInfo(Instant.now(), null, null, null),
                null,
                null
        );

        upsert(entity1);
        upsert(entity2);

        assertThat(findLastConfig(TestData.CONFIG_PATH_ABC, ArcBranch.trunk()))
                .contains(entity1);

        assertThat(findLastConfig(TestData.CONFIG_PATH_ABC, TestData.TRUNK_R3))
                .contains(entity2);

    }

    @Test
    void testUpdateSecurityState() {

        Instant now = Instant.now();

        ConfigEntity entity = new ConfigEntity(
                TestData.CONFIG_PATH_ABE,
                TestData.TRUNK_R4,
                ConfigStatus.SECURITY_PROBLEM,
                List.of(new ConfigProblem(ConfigProblem.Level.WARN, "Ничего страшного", "Реально не обращай " +
                        "внимание")),
                new TreeMap<>(Map.of(TestData.TASK_ID, TestData.REVISION)),
                TestData.NO_TOKEN_SECURITY_STATE,
                new ConfigCreationInfo(
                        now,
                        TestData.TRUNK_R3,
                        TestData.TRUNK_R3,
                        null
                ),
                null,
                null
        );

        upsert(entity);
        assertThat(findConfig(TestData.CONFIG_PATH_ABE, TestData.TRUNK_R4))
                .contains(entity);

        db.currentOrTx(() ->
                db.configHistory().updateSecurityState(
                        TestData.CONFIG_PATH_ABE,
                        TestData.TRUNK_R4,
                        TestData.VALID_SECURITY_STATE));

        ConfigEntity expected = new ConfigEntity(
                TestData.CONFIG_PATH_ABE,
                TestData.TRUNK_R4,
                ConfigStatus.READY,
                List.of(new ConfigProblem(ConfigProblem.Level.WARN, "Ничего страшного", "Реально не обращай внимание")),
                new TreeMap<>(Map.of(TestData.TASK_ID, TestData.REVISION)),
                TestData.VALID_SECURITY_STATE,
                new ConfigCreationInfo(
                        now,
                        TestData.TRUNK_R3,
                        TestData.TRUNK_R3,
                        null
                ),
                null,
                null
        );

        assertThat(findConfig(TestData.CONFIG_PATH_ABE, TestData.TRUNK_R4))
                .contains(expected);
    }

    private void upsert(ConfigEntity configEntity) {
        db.currentOrTx(() -> db.configHistory().save(configEntity));
    }

    private Optional<ConfigEntity> findConfig(Path configPath, OrderedArcRevision revision) {
        return db.currentOrReadOnly(() -> db.configHistory().findById(configPath, revision));
    }

    private Optional<ConfigEntity> findLastConfig(Path configPath, ArcBranch branch) {
        return db.currentOrReadOnly(() -> db.configHistory().findLastConfig(configPath, branch));
    }

    private Optional<ConfigEntity> findLastConfig(Path configPath, OrderedArcRevision revision) {
        return db.currentOrReadOnly(() -> db.configHistory().findLastConfig(configPath, revision));
    }


}
