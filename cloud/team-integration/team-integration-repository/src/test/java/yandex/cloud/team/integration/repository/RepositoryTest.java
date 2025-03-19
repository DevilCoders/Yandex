package yandex.cloud.team.integration.repository;

import java.util.List;

import org.assertj.core.api.Assertions;
import org.junit.Rule;
import org.junit.Test;
import yandex.cloud.audit.OperationDb;
import yandex.cloud.iam.repository.tracing.QueryTraceRule;
import yandex.cloud.model.operation.Operation;
import yandex.cloud.repository.BaseDb;
import yandex.cloud.repository.db.list.ListRequest;
import yandex.cloud.repository.db.list.ListResult;
import yandex.cloud.task.TaskDb;
import yandex.cloud.task.model.Task;

public class RepositoryTest {

    @Rule
    public final RepositoryRule repositoryRule = new RepositoryRule(TeamIntegrationEntitiesHelper.collectEntities(List.of(
    )));
    @Rule
    public final QueryTraceRule queryTraceRule = new QueryTraceRule(this);

    @Test
    public void listOperations() {
        ListResult<Operation> result = repositoryRule.tx(() -> OperationDb.current()
                .operations()
                .list(ListRequest.builder(Operation.class).build())
        );
        Assertions.assertThat(result.isEmpty()).isTrue();
    }

    @Test
    public void listTasks() {
        ListResult<Task> result = repositoryRule.tx(() -> BaseDb.current(TaskDb.class)
                .tasks()
                .list(ListRequest.builder(Task.class).build())
        );
        Assertions.assertThat(result.isEmpty()).isTrue();
    }

}
