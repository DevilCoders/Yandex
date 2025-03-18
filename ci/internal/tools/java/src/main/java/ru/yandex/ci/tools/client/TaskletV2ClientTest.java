package ru.yandex.ci.tools.client;

import com.google.protobuf.ByteString;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.taskletv2.TaskletV2Client;
import ru.yandex.ci.core.spring.clients.TaskletV2ClientConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;
import ru.yandex.tasklet.api.v2.DataModel;
import ru.yandex.tasklet.api.v2.DataModel.EExecutionStatus;
import ru.yandex.tasklet.api.v2.DataModel.ExecutionRequirements;
import ru.yandex.tasklet.api.v2.SchemaRegistryServiceOuterClass.GetSchemaRequest;
import ru.yandex.tasklet.api.v2.TaskletServiceOuterClass;
import ru.yandex.tasklet.api.v2.TaskletServiceOuterClass.ExecuteRequest;
import ru.yandex.tasklet.api.v2.TaskletServiceOuterClass.GetExecutionRequest;
import ru.yandex.tasklet.api.v2.WellKnownStructures;

@Slf4j
@Import(TaskletV2ClientConfig.class)
@Configuration
public class TaskletV2ClientTest extends AbstractSpringBasedApp {

    @Autowired
    TaskletV2Client taskletV2Client;

    @Override
    protected void run() throws Exception {
        getSchema();
    }

    void getSchema() {
        var schema = taskletV2Client.getSchema(
                GetSchemaRequest.newBuilder()
                        .setHash("16f28dc9fc4785f4c1da088151020bad")
                        .build());
        log.info("Schema meta: {}", schema.getMeta());

        var label = taskletV2Client.getLabel(
                TaskletServiceOuterClass.GetLabelRequest.newBuilder()
                        .setNamespace("cidemo")
                        .setTasklet("furniture_factory")
                        .setLabel("release")
                        .build());
        log.info("Label: {}", label.getLabel());
    }

    @SuppressWarnings("BusyWait")
    void execute() throws InterruptedException {
        var data = WellKnownStructures.GenericBinary.newBuilder()
                .setPayload(ByteString.copyFromUtf8("{\"result_data\": \"my-value\"}"))
                .build();

        var executeRequest = ExecuteRequest.newBuilder()
                .setNamespace("test-tasklets")
                .setTasklet("dummy_java_tasklet")
                .setLabel("version1")
                .setInput(DataModel.ExecutionInput.newBuilder()
                        .setSerializedData(data.toByteString()))
                .setRequirements(ExecutionRequirements.newBuilder()
                        .setAccountId("CI"))
                .build();
        var executor = taskletV2Client.getExecutor(System.getenv("CI_TOKEN"));

        var execution = executor.execute(executeRequest).getExecution();
        log.info("Execution response: \n{}", execution);

        var executionId = execution.getMeta().getId();
        log.info("Execution id: {}", executionId);

        while (true) {
            var status = execution.getStatus().getStatus();
            if (status == EExecutionStatus.E_EXECUTION_STATUS_INVALID) {
                throw new RuntimeException("Unable to start tasklet: " +
                        execution.getStatus().getError().getDescription());
            } else if (status == EExecutionStatus.E_EXECUTION_STATUS_FINISHED) {
                break;
            }

            Thread.sleep(5000);
            var getExecutionRequest = GetExecutionRequest.newBuilder()
                    .setId(executionId)
                    .build();
            execution = executor.getExecution(getExecutionRequest).getExecution();
            log.info("Execution response: \n{}", execution);
        }

        log.info("Last execution status is {}", execution.getStatus().getStatus());
    }

    public static void main(String[] args) {
        startAndStopThisClass(args);
    }

}
