package ru.yandex.ci.engine.branch;

import java.util.NoSuchElementException;
import java.util.Objects;
import java.util.Optional;

import javax.annotation.Nullable;

import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.arc.branch.BranchInfo;
import ru.yandex.ci.core.db.CiMainDb;

/**
 * Обход интервала ревизий интервалами, приходящими на одну ветку.
 * Например, для отведенной на ревизии r5 ветки и интервала от rXX этой ветки до rYYY транка,
 * интервал будет разбит на два: rXX - null, r5 - rYYY.
 */
@Slf4j
@RequiredArgsConstructor
public class BranchTraverseService {
    private final CiMainDb db;

    public Optional<OrderedArcRevision> findBaseRevision(ArcBranch branch) {
        if (branch.isTrunk()) {
            return Optional.empty();
        }

        var branchInfo = db.branches().get(BranchInfo.Id.of(branch));
        return Optional.of(branchInfo.getBaseRevision());
    }

    public OrderedArcRevision getBaseRevision(ArcBranch branch) {
        return findBaseRevision(branch).orElseThrow(() ->
                new NoSuchElementException(String.format("Unable to find base revision of %s", branch)));
    }

    public void traverse(OrderedArcRevision fromRevision,
                         @Nullable OrderedArcRevision toRevisionExclusive,
                         RangeConsumer rangeConsumer) {
        do {
            log.info("Traverse {} -> {}", fromRevision, toRevisionExclusive);
            if (toRevisionExclusive != null && toRevisionExclusive.getBranch().equals(fromRevision.getBranch())) {
                if (!Objects.equals(fromRevision, toRevisionExclusive)) {
                    log.info("Consuming {} -> {}", fromRevision, toRevisionExclusive);
                    rangeConsumer.consume(fromRevision, toRevisionExclusive);
                } else {
                    log.info("Empty range {} -> {}, skipped", fromRevision, toRevisionExclusive);
                }
                break;
            } else {
                // process whole branch from fromRevision
                log.info("Consuming {} -> null (whole branch)", fromRevision);
                boolean shouldBreak = rangeConsumer.consume(fromRevision, null);
                if (shouldBreak) {
                    break;
                }
                fromRevision = findBaseRevision(fromRevision.getBranch()).orElse(null);
            }
        } while (fromRevision != null);

        log.info("Traversing finished");
    }

    public interface RangeConsumer {
        /**
         * Обработка интервала коммитов на интервале одной ветки.
         * Если обе ревизии определены, то их {@link OrderedArcRevision#getBranch()} равны.
         * @param fromRevision более поздняя ревизия
         * @param toRevision более ранняя ревизия или null - значит до конца ветки.
         * @return true если нужно прервать итерацию
         */
        boolean consume(OrderedArcRevision fromRevision, @Nullable OrderedArcRevision toRevision);
    }
}
