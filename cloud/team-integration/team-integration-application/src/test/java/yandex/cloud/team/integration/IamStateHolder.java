package yandex.cloud.team.integration;

import yandex.cloud.fake.OperationFactory;
import yandex.cloud.fake.iam.FakeIamServer;
import yandex.cloud.fake.template.StateHolder;
import yandex.cloud.util.Ids;

class IamStateHolder implements StateHolder<FakeIamServer.State> {

    private final OperationFactory operationFactory = new OperationFactory(this::generateId);

    private final FakeIamServer.State state;

    public IamStateHolder() {
        state = FakeIamServer.State.create(this::generateId, this::generateId, this::generateId);
    }

    @Override
    public FakeIamServer.State getState() {
        return state;
    }

    @Override
    public OperationFactory getOperationFactory() {
        return operationFactory;
    }

    @Override
    public String generateId() {
        return Ids.generateId("foo");
    }

}
