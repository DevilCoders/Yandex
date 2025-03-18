package ru.yandex.ci.engine.pr;

import javax.annotation.Nonnull;

import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.commune.bazinga.BazingaTaskManager;

@Slf4j
@RequiredArgsConstructor
public class PullRequestCommentService {

    @Nonnull
    private final BazingaTaskManager taskManager;

    public void scheduleCreatePrComment(long reviewRequestId, String comment) {
        log.info("Scheduling comment for review {}: {}", reviewRequestId, comment);
        taskManager.schedule(new CreatePrCommentTask(reviewRequestId, comment));
    }
}
