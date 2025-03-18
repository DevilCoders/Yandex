package ru.yandex.ci.engine.launch.auto;

import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import yandex.cloud.repository.db.exception.EntityAlreadyExistsException;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.discovery.CommitDiscoveryProgress;
import ru.yandex.ci.core.discovery.DiscoveryType;

@Slf4j
@RequiredArgsConstructor
public class DiscoveryProgressService {

    private final CiMainDb db;

    public void markAsDiscovered(OrderedArcRevision arcRevision, DiscoveryType discoveryType) {
        if (!arcRevision.getBranch().isTrunk()) {
            log.info("{} doesn't marked as discovered, cause commit is not in trunk", arcRevision);
            return;
        }
        log.info("{} marking as discovered by {}", arcRevision, discoveryType);
        db.currentOrTx(() -> {
            var updatedProgress = db.commitDiscoveryProgress()
                    .find(arcRevision.getCommitId())
                    .map(it -> it.withDiscoveryFinished(discoveryType))
                    .orElseGet(() -> CommitDiscoveryProgress.of(arcRevision).withDiscoveryFinished(discoveryType));
            db.commitDiscoveryProgress().save(updatedProgress);
            log.info("{} marked as ready for checking previous commits", arcRevision.getCommitId());
        });
    }

    public void createCommitDiscoveryProgress(OrderedArcRevision arcRevision) {
        if (!arcRevision.getBranch().isTrunk()) {
            log.info("discovery progress NOT created for {}, cause branch is not trunk", arcRevision);
            return;
        }

        try {
            db.currentOrTx(() -> {
                if (db.commitDiscoveryProgress().find(arcRevision.getCommitId()).isEmpty()) {
                    // we don't wont to update commit status, so we use "insert" instead of "save"
                    db.commitDiscoveryProgress().save(CommitDiscoveryProgress.of(arcRevision));
                }
            });
            log.info("discovery progress created for {}", arcRevision);
        } catch (EntityAlreadyExistsException e) {
            log.warn("discovery progress NOT created for {}", arcRevision, e);
        }
    }
}
