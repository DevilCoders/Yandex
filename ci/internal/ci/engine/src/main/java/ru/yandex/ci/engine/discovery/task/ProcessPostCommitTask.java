package ru.yandex.ci.engine.discovery.task;

import java.time.Duration;

import javax.annotation.Nullable;

import lombok.EqualsAndHashCode;
import lombok.ToString;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.ci.common.bazinga.AbstractOnetimeTask;
import ru.yandex.ci.common.bazinga.StringUniqueIdentifierConverter;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.engine.discovery.DiscoveryServicePostCommits;
import ru.yandex.commune.bazinga.scheduler.ActiveUidBehavior;
import ru.yandex.commune.bazinga.scheduler.ActiveUidDropType;
import ru.yandex.commune.bazinga.scheduler.ActiveUidDuplicateBehavior;
import ru.yandex.commune.bazinga.scheduler.ActiveUniqueIdentifierConverter;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.lang.NonNullApi;
import ru.yandex.misc.bender.annotation.BenderBindAllFields;

@NonNullApi
public class ProcessPostCommitTask extends AbstractOnetimeTask<ProcessPostCommitTask.Params> {

    private static final Logger log = LoggerFactory.getLogger(ProcessPostCommitTask.class);

    private DiscoveryServicePostCommits discoveryServicePostCommits;

    public ProcessPostCommitTask(DiscoveryServicePostCommits discoveryServicePostCommits) {
        super(Params.class);
        this.discoveryServicePostCommits = discoveryServicePostCommits;
    }

    public ProcessPostCommitTask(Params params) {
        super(params);
    }


    @Override
    protected void execute(Params params, ExecutionContext context) {
        log.info("Processing: {} {}", params.getBranch(), params.getRevision());
        discoveryServicePostCommits.processPostCommit(params.getBranch(), params.getRevision());
        log.info("Processed: {} {}", params.getBranch(), params.getRevision());
    }

    @Override
    public ActiveUidBehavior activeUidBehavior() {
        return new ActiveUidBehavior(ActiveUidDropType.WHEN_RUNNING, ActiveUidDuplicateBehavior.DO_NOTHING);
    }

    @Nullable
    @Override
    public Class<? extends ActiveUniqueIdentifierConverter<?, ?>> getActiveUidConverter() {
        return ActiveUniqueIdentifierConverterImpl.class;
    }

    @Override
    public Duration getTimeout() {
        return Duration.ofMinutes(10);
    }


    @NonNullApi
    public static class ActiveUniqueIdentifierConverterImpl extends StringUniqueIdentifierConverter<Params> {
        @Override
        protected String convertToString(Params params) {
            return params.getBranch().asString() + ":" + params.getRevision().getCommitId();
        }
    }

    @ToString
    @BenderBindAllFields
    @EqualsAndHashCode
    public static final class Params {
        private final String commitHash;
        private final String branch;

        public Params(ArcBranch branch, ArcRevision revision) {
            this.branch = branch.toString();
            this.commitHash = revision.getCommitId();
        }

        public ArcRevision getRevision() {
            return ArcRevision.of(commitHash);
        }

        public ArcBranch getBranch() {
            return ArcBranch.ofBranchName(branch);
        }

    }
}
