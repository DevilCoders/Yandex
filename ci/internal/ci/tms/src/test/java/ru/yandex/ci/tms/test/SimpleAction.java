package ru.yandex.ci.tms.test;

import ru.yandex.ci.engine.test.schema.Simple;
import ru.yandex.ci.engine.test.schema.SimpleData;
import ru.yandex.tasklet.Result;
import ru.yandex.tasklet.TaskletAction;
import ru.yandex.tasklet.TaskletContext;
import ru.yandex.tasklet.api.v2.WellKnownStructures;

class SimpleAction implements TaskletAction<SimpleData, SimpleData> {
    static final String SIMULATE_USER_ERROR = "user-error";
    static final String SIMULATE_SERVER_ERROR = "server-error";

    @Override
    public Result<SimpleData> execute(SimpleData simpleData, TaskletContext context) throws Exception {
        var value = simpleData.getSimpleDataField().getSimpleString();
        if (SIMULATE_USER_ERROR.equals(value)) {
            return Result.ofError(WellKnownStructures.UserError.newBuilder()
                    .setIsTransient(false)
                    .setDescription("User error")
                    .build());
        } else if (SIMULATE_SERVER_ERROR.equals(value)) {
            throw new RuntimeException("Runtime exception");
        } else {
            return Result.of(SimpleData.newBuilder()
                    .setSimpleDataField(Simple.newBuilder()
                            .setSimpleString("return-" + value))
                    .build());
        }
    }
}
