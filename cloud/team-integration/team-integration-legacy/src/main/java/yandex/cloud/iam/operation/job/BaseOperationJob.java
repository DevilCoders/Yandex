package yandex.cloud.iam.operation.job;

import lombok.extern.log4j.Log4j2;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.audit.OperationContextAwareJob;
import yandex.cloud.iam.exception.IamException;
import yandex.cloud.iam.operation.OperationProcess;
import yandex.cloud.model.operation.OperationContext;
import yandex.cloud.model.operation.OperationError;
import yandex.cloud.model.operation.OperationErrorData;
import yandex.cloud.model.operation.OperationResponse;
import yandex.cloud.repository.db.exception.UnavailableException;

@Log4j2
public abstract class BaseOperationJob<T> extends OperationContextAwareJob {

    @NotNull
    private T processState;

    private transient RuntimeException exception = null;

    private transient OperationProcess<T> process = null;

    protected BaseOperationJob(@NotNull T processState) {
        this.processState = processState;
    }

    @Override
    public String getId() {
        return JobUtil.composeJobId(this, callerOperationId.getValue());
    }

    @Override
    protected final void prepareInContext() {
        process = createProcess(processState);
        process.prepare();
    }

    @Override
    protected final void runInContext() {
        try {
            process.run();
        } catch (UnavailableException | IamException e) {
            exception = e;
        }
    }

    @Override
    protected final void doneInContext() {
        if (exception == null) {
            try {
                processState = process.done();
            } catch (IamException e) {
                exception = e;
            }
        }

        if (exception == null) {
            if (process.isDone()) {
                if (process.isSucceeded()) {
                    OperationContext.get().update(op -> op.done(getOperationResponse()));
                } else {
                    OperationErrorData operationError = process.getOperationError();
                    final OperationError error;
                    if (operationError instanceof OperationError) {
                        error = (OperationError) operationError;
                    } else {
                        error = new OperationError(operationError.getCode(), operationError.getMessage());
                    }
                    OperationContext.get().update(op -> op.fail(error));
                }
            } else {
                JobUtil.retryRequired(process.getRetryBackoff());
            }
        } else {
            if (exception instanceof UnavailableException) {
                log.warn(String.format("[op=%s] Retry operation job", callerOperationId.getValue()), exception);

                JobUtil.retryRequired(process.getRetryBackoff());
            } else {
                log.error(String.format("[op=%s] Operation job failed", callerOperationId.getValue()), exception);

                OperationContext.get().update(op -> op.fail(exception.getMessage()));
            }
        }
    }

    protected abstract @NotNull OperationProcess<T> createProcess(T processState);

    protected OperationResponse getOperationResponse() {
        return process.getOperationResponse();
    }

}
