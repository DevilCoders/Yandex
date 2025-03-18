package ru.yandex.ci.engine.discovery.arc_reflog;

import java.time.Duration;

import javax.annotation.Nullable;

import com.google.common.base.Strings;
import lombok.AllArgsConstructor;
import lombok.EqualsAndHashCode;

import ru.yandex.ci.common.bazinga.AbstractOnetimeTask;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.misc.bender.annotation.BenderBindAllFields;

public class ProcessArcReflogRecordTask extends AbstractOnetimeTask<ProcessArcReflogRecordTask.ReflogRecord> {
    private final ReflogProcessorService reflogProcessorService;

    public ProcessArcReflogRecordTask(ReflogProcessorService reflogProcessorService) {
        super(ReflogRecord.class);
        this.reflogProcessorService = reflogProcessorService;
    }

    public ProcessArcReflogRecordTask(ArcBranch branch, ArcRevision revision, ArcRevision previousRevision) {
        super(new ReflogRecord(branch.asString(), revision.getCommitId(), previousRevision.getCommitId()));
        reflogProcessorService = null;
    }

    @Override
    protected void execute(ReflogRecord record, ExecutionContext context) throws Exception {
        reflogProcessorService.processReflogRecord(
                record.getBranch(),
                record.getRevision(),
                record.getPreviousRevision()
        );
    }

    @Override
    public Duration getTimeout() {
        return Duration.ofMinutes(20);
    }

    public static boolean acceptBranch(ArcBranch arcBranch) {
        return ReflogProcessorService.acceptBranch(arcBranch);
    }

    @BenderBindAllFields
    @AllArgsConstructor
    @EqualsAndHashCode
    public static final class ReflogRecord {
        private final String branch;
        private final String revision;
        @Nullable
        private final String previousRevision;

        public ArcRevision getRevision() {
            return ArcRevision.of(revision);
        }

        @Nullable
        public ArcRevision getPreviousRevision() {
            return Strings.isNullOrEmpty(previousRevision) ? null : ArcRevision.of(previousRevision);
        }

        public ArcBranch getBranch() {
            return ArcBranch.ofBranchName(branch);
        }
    }
}
