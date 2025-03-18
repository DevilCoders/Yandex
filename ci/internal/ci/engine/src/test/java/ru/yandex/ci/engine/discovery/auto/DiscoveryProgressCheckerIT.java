package ru.yandex.ci.engine.discovery.auto;

import java.time.Instant;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.EnumSource;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.core.discovery.CommitDiscoveryProgress;
import ru.yandex.ci.core.discovery.DiscoveryType;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.EngineTestBase;
import ru.yandex.ci.engine.launch.auto.DiscoveryProgressChecker;

import static org.assertj.core.api.Assertions.assertThat;

public class DiscoveryProgressCheckerIT extends EngineTestBase {

    @Autowired
    DiscoveryProgressChecker checker;

    @ParameterizedTest
    @EnumSource(DiscoveryType.class)
    void checkDiscovery(DiscoveryType discoveryType) {
        // prepare environment
        CommitDiscoveryProgress r0Progress = CommitDiscoveryProgress.builder()
                .arcRevision(TestData.TRUNK_R1)
                .build();
        CommitDiscoveryProgress r1Progress = CommitDiscoveryProgress.builder()
                .arcRevision(TestData.TRUNK_R2)
                .build();

        CommitDiscoveryProgress r2Progress = CommitDiscoveryProgress.builder()
                .arcRevision(TestData.TRUNK_R3)
                .build();

        db.currentOrTx(() -> {
            clock.setTime(Instant.parse("2015-01-01T10:00:00.000Z"));
            db.arcCommit().save(TestData.TRUNK_COMMIT_1.toBuilder()
                    .createTime(Instant.parse("2015-01-01T10:00:00.000Z"))
                    .build()
            );
            db.arcCommit().save(TestData.TRUNK_COMMIT_2.toBuilder()
                    .createTime(Instant.parse("2016-01-01T10:00:00.000Z"))
                    .parents(List.of(TestData.TRUNK_COMMIT_1.getCommitId()))
                    .build()
            );
            db.arcCommit().save(TestData.TRUNK_COMMIT_3.toBuilder()
                    .createTime(Instant.parse("2017-01-01T10:00:00.000Z"))
                    .parents(List.of(TestData.TRUNK_COMMIT_2.getCommitId()))
                    .build()
            );
            db.commitDiscoveryProgress().save(List.of(r0Progress, r1Progress, r2Progress));
        });

        assertThat(fetchDiscoveryFinishedForParentsField(discoveryType)).isEqualTo(Map.of(
                "r1", false,
                "r2", false,
                "r3", false
        ));
        // start testing
        checker.check();
        assertThat(fetchDiscoveryFinishedForParentsField(discoveryType)).isEqualTo(Map.of(
                "r1", false,
                "r2", false,
                "r3", false
        ));

        clock.setTime(Instant.parse("2015-01-02T10:00:00.000Z"));
        checker.check();
        assertThat(fetchDiscoveryFinishedForParentsField(discoveryType)).isEqualTo(Map.of(
                "r1", true,
                "r2", false,
                "r3", false
        ));

        db.currentOrTx(() -> db.commitDiscoveryProgress().save(r1Progress.withDiscoveryFinished(discoveryType)));
        checker.check();
        assertThat(fetchDiscoveryFinishedForParentsField(discoveryType)).isEqualTo(Map.of(
                "r1", true,
                // r2 is false, cause r1.graphDiscoveryFinished == false
                "r2", false,
                "r3", false
        ));

        db.currentOrTx(() -> {
            db.commitDiscoveryProgress().find(r0Progress.getId().getCommitId())
                    .map(it -> it.withDiscoveryFinished(discoveryType))
                    .ifPresent(db.commitDiscoveryProgress()::save);
            db.commitDiscoveryProgress().find(r2Progress.getId().getCommitId())
                    .map(it -> it.withDiscoveryFinished(discoveryType))
                    .ifPresent(db.commitDiscoveryProgress()::save);
        });
        checker.check();
        assertThat(fetchDiscoveryFinishedForParentsField(discoveryType)).isEqualTo(Map.of(
                "r1", true,
                "r2", true,
                "r3", true
        ));
    }

    private Map<String, Boolean> fetchDiscoveryFinishedForParentsField(DiscoveryType discoveryType) {
        return db.currentOrTx(() ->
                db.commitDiscoveryProgress().findAll()
                        .stream()
                        .collect(Collectors.toMap(
                                it -> it.getId().getCommitId(),
                                it -> it.isDiscoveryFinishedForParents(discoveryType)
                        ))
        );
    }

}
