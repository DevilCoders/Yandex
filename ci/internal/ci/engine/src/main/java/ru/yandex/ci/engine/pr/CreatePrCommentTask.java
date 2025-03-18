package ru.yandex.ci.engine.pr;

import java.time.Duration;

import javax.annotation.Nonnull;

import lombok.Value;
import lombok.extern.slf4j.Slf4j;
import org.apache.commons.lang3.StringUtils;

import ru.yandex.ci.client.arcanum.ArcanumClientImpl;
import ru.yandex.ci.common.bazinga.AbstractOnetimeTask;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.misc.bender.annotation.BenderBindAllFields;

@Slf4j
public class CreatePrCommentTask extends AbstractOnetimeTask<CreatePrCommentTask.Params> {
    private ArcanumClientImpl arcanumClient;
    private static final int COMMENT_LOG_TRIM_LENGTH = 256;

    public CreatePrCommentTask(ArcanumClientImpl arcanumClient) {
        super(Params.class);
        this.arcanumClient = arcanumClient;
    }

    public CreatePrCommentTask(long reviewRequestId, String comment) {
        super(new Params(reviewRequestId, comment));
    }

    @Override
    protected void execute(Params params, ExecutionContext context) throws Exception {
        log.info("Creating comment '{}' in pr {}",
                StringUtils.abbreviate(params.getComment(), COMMENT_LOG_TRIM_LENGTH),
                params.getReviewRequestId()
        );

        arcanumClient.createReviewRequestComment(params.getReviewRequestId(), params.getComment());
    }

    @Override
    public Duration getTimeout() {
        return Duration.ofSeconds(10);
    }

    @Value
    @BenderBindAllFields
    public static class Params {
        long reviewRequestId;
        @Nonnull
        String comment;
    }
}
