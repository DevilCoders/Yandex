package yandex.cloud.ti.operation;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.audit.OperationDb;
import yandex.cloud.di.Configuration;

public class OperationConfiguration extends Configuration {

    @Override
    protected void configure() {
        put(OperationService.class, this::operationService);
    }

    protected @NotNull OperationService operationService() {
        return new OperationServiceImpl(
                getOperationDb()
        );
    }

    protected @NotNull OperationDb getOperationDb() {
        return OperationDb.current();
    }

}
