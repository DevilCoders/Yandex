package ru.yandex.ci.event.arc;

import com.google.protobuf.TextFormat;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.arc.api.Message;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.engine.discovery.arc_reflog.ProcessArcReflogRecordTask;
import ru.yandex.commune.bazinga.BazingaTaskManager;

public class ArcEventService {

    private static final Logger log = LoggerFactory.getLogger(ArcEventService.class);

    private final BazingaTaskManager taskManager;

    public ArcEventService(BazingaTaskManager taskManager) {
        this.taskManager = taskManager;
    }

    public void processEvent(Message.ReflogRecord reflogRecord) {
        log.info("Processing arc event: {}", TextFormat.shortDebugString(reflogRecord));

        ArcBranch branch = ArcBranch.ofBranchName(reflogRecord.getName());
        if (!ProcessArcReflogRecordTask.acceptBranch(branch)) {
            log.info("Skipping event. Unsuitable branch: {}", branch);
            return; // -- do not schedule event
        }

        Message.ReflogRecord.ReflogType type = reflogRecord.getType();
        switch (type) {
            case TypeCreate:
            case TypeFastForwardPush:
            case TypeFastForwardFill:
                break;
            default:
                log.info("Branch {} update event skipped by type {}", branch, type);
                return;
        }
        ArcRevision revision = ArcRevision.of(reflogRecord.getAfterOid());
        ArcRevision previousRevision = ArcRevision.of(reflogRecord.getBeforeOid());
        taskManager.schedule(new ProcessArcReflogRecordTask(branch, revision, previousRevision));
    }


}
