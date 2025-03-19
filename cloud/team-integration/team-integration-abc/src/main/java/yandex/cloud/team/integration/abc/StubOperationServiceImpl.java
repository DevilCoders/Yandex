package yandex.cloud.team.integration.abc;

import java.util.Optional;
import java.util.function.Predicate;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.auth.SecurityContext;
import yandex.cloud.model.operation.Operation;
import yandex.cloud.model.operation.OperationData;
import yandex.cloud.ti.abc.AbcServiceCloudCreateOperationReference;
import yandex.cloud.ti.abc.AbcServiceCloudStubOperationReference;
import yandex.cloud.ti.abc.repo.AbcIntegrationRepository;
import yandex.cloud.ti.operation.OperationService;

public class StubOperationServiceImpl implements StubOperationService {

    private final @NotNull AbcIntegrationRepository abcIntegrationRepository;
    private final @NotNull OperationService operationService;


    public StubOperationServiceImpl(
            @NotNull AbcIntegrationRepository abcIntegrationRepository,
            @NotNull OperationService operationService
    ) {
        this.abcIntegrationRepository = abcIntegrationRepository;
        this.operationService = operationService;
    }


    @Override
    public @NotNull Operation createStub(
            @NotNull Operation mainOperation
    ) {
        var stubOperation = operationService.saveOperation(Operation
                .create(null, SecurityContext.Current.userId())
                .withMetadata(mainOperation.getMetadata())
                .withDescription(mainOperation.getDescription())
        );
        abcIntegrationRepository.saveStubOperation(
                stubOperation.getId(),
                mainOperation.getId()
        );
        return stubOperation;
    }

    @Override
    public @NotNull Operation processStub(
            @NotNull Operation operation
    ) {
        AbcServiceCloudStubOperationReference stubOperationRef = abcIntegrationRepository.findStubOperationByOperationId(
                operation.getId()
        );
        if (stubOperationRef != null) {
            var createOperation = operationService.findOperation(stubOperationRef.createOperationId());
            if (createOperation == null) {
                throw OperationNotFoundException.of(stubOperationRef.createOperationId().getValue());
            }
            if (createOperation.isDone()) {
                return operationService.saveOperation(createOperation.isFailed()
                        ? operation.fail(createOperation.getError())
                        : operation.done(createOperation.getResponse())
                );
            }
        }
        return operation;
    }

    @Override
    public Optional<Operation> findMainOperation(
            long abcId
    ) {
        return Optional
                .ofNullable(abcIntegrationRepository.findCreateOperationByAbcServiceId(abcId))
                .map(AbcServiceCloudCreateOperationReference::createOperationId)
                .map(operationService::findOperation)
                .filter(Predicate.not(OperationData::isFailed));
    }

    @Override
    public void saveMainOperation(
            long abcId,
            @NotNull Operation mainOperation
    ) {
        abcIntegrationRepository.saveCreateOperation(
                abcId,
                mainOperation.getId()
        );
    }

}
