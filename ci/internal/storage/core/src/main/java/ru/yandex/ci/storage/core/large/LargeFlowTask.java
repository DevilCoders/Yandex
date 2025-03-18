package ru.yandex.ci.storage.core.large;

import java.time.Duration;

import javax.annotation.Nonnull;

import lombok.ToString;

import ru.yandex.ci.common.bazinga.AbstractOnetimeTask;
import ru.yandex.ci.common.bazinga.StringUniqueIdentifierConverter;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.check_task.LargeTaskEntity;
import ru.yandex.commune.bazinga.scheduler.ActiveUniqueIdentifierConverter;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.commune.bazinga.ydb.storage.util.YdbUtils;
import ru.yandex.lang.NonNullApi;
import ru.yandex.misc.bender.annotation.BenderBindAllFields;

public class LargeFlowTask extends AbstractOnetimeTask<LargeFlowTask.Params> {
    private LargeFlowStartService largeFlowStartService;

    public LargeFlowTask(@Nonnull LargeFlowStartService largeFlowStartService) {
        super(LargeFlowTask.Params.class);
        this.largeFlowStartService = largeFlowStartService;
    }

    public LargeFlowTask(@Nonnull LargeTaskEntity.Id largeTaskId) {
        super(new Params(largeTaskId));
    }

    @Override
    public Class<? extends ActiveUniqueIdentifierConverter<?, ?>> getActiveUidConverter() {
        return LargeFlowTask.ActiveUniqueIdentifierConverterImpl.class;
    }

    @Override
    protected void execute(Params params, ExecutionContext context) {
        largeFlowStartService.startLargeFlow(params.getLargeTaskId());
    }

    @Override
    public Duration getTimeout() {
        return Duration.ofMinutes(5);
    }

    @ToString
    @BenderBindAllFields
    public static class Params {
        private final BazingaIterationId id;
        private final Common.CheckTaskType type;
        private final int index;

        public Params(LargeTaskEntity.Id largeTaskId) {
            this.id = new BazingaIterationId(largeTaskId.getIterationId());
            this.type = largeTaskId.getCheckTaskType();
            this.index = largeTaskId.getIndex();
        }

        public Common.CheckTaskType getType() {
            return type;
        }

        public LargeTaskEntity.Id getLargeTaskId() {
            return new LargeTaskEntity.Id(id.getIterationId(), type, index);
        }
    }

    @NonNullApi
    public static class ActiveUniqueIdentifierConverterImpl extends
            StringUniqueIdentifierConverter<LargeFlowTask.Params> {
        @Override
        protected String convertToString(LargeFlowTask.Params params) {
            return YdbUtils.EXCLUDE_UNIQUE_ID_CHECK + params.getLargeTaskId();
        }
    }

}
