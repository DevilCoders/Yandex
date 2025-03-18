package ru.yandex.ci.tools.flows;

import java.util.List;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.api.storage.StorageApi;
import ru.yandex.ci.client.ci.CiClient;
import ru.yandex.ci.storage.core.spring.ClientsConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Configuration
@Import(ClientsConfig.class)
public class CancelFlow extends AbstractSpringBasedApp {

    @Autowired
    CiClient client;

    @Override
    protected void run() {
        var numbers = List.of(3311);

        for (var number : numbers) {
            client.cancelFlow(StorageApi.CancelFlowRequest.newBuilder()
                    .setFlowProcessId(Common.FlowProcessId.newBuilder()
                            .setDir("junk/miroslav2/ci/run-cli")
                            .setId("run-cli")
                            .build())
                    .setNumber(number)
                    .build());
        }
    }

    public static void main(String[] args) {
        startAndStopThisClass(args);
    }

}
