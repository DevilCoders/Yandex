package yandex.cloud.iam.operation;

import lombok.extern.log4j.Log4j2;
import yandex.cloud.audit.AbstractOperationMethod;
import yandex.cloud.iam.exception.IamException;
import yandex.cloud.model.operation.OperationContext;

@Log4j2
public abstract class IamAbstractOperationMethod extends AbstractOperationMethod {
    @Override
    protected final void runInOperationContext() {
        try {
            runOperation();
        } catch (IamException e) {
            log.error("Operation failed", e);
            getOperationContext().update(op -> op.fail(e.getMessage()));
        }
    }

    protected abstract void runOperation();

    protected OperationContext getOperationContext() {
        return OperationContext.get();
    }
}
