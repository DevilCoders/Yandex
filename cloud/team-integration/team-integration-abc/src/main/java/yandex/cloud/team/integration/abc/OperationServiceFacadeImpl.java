package yandex.cloud.team.integration.abc;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.iam.service.TransactionHandler;
import yandex.cloud.model.operation.Operation;
import yandex.cloud.ti.abc.repo.AbcIntegrationRepository;
import yandex.cloud.ti.operation.OperationService;

public class OperationServiceFacadeImpl implements OperationServiceFacade {

    private final @NotNull AbcIntegrationRepository abcIntegrationRepository;
    private final @NotNull StubOperationService stubOperationService;
    private final @NotNull OperationService operationService;


    public OperationServiceFacadeImpl(
            @NotNull AbcIntegrationRepository abcIntegrationRepository,
            @NotNull StubOperationService stubOperationService,
            @NotNull OperationService operationService
    ) {
        this.abcIntegrationRepository = abcIntegrationRepository;
        this.stubOperationService = stubOperationService;
        this.operationService = operationService;
    }


    @Override
    public Operation getOperation(
            @NotNull String operationId
    ) {
        return TransactionHandler.runInTx(() -> {
            var operation = operationService.findOperation(new Operation.Id(operationId));
            if (operation == null) {
                throw OperationNotFoundException.of(operationId);
            }
            if (!operation.isDone()) {
                operation = stubOperationService.processStub(operation);
            }
            return operation;
        });
    }

}
