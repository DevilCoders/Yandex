package ru.yandex.ci.tools.flows;

import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.engine.spring.AutoReleaseConfig;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.di.ResourceService;
import ru.yandex.ci.flow.engine.runtime.state.calculator.FlowResourcesAccessor;
import ru.yandex.ci.flow.engine.runtime.state.calculator.FlowStateCalculator;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Configuration
@Import(AutoReleaseConfig.class)
public class LoadFlowRuntime extends AbstractSpringBasedApp {

    @Autowired
    CiDb db;

    @Autowired
    FlowStateCalculator calculator;

    @Autowired
    ResourceService resourceService;

    @Override
    protected void run() {
        var id = FlowLaunchId.of("cdd69534af28116f1ce7737354c44442ad9a391488143e180d415387207cfcee");
        var flowLaunch = db.currentOrReadOnly(() -> db.flowLaunch().get(id));
        log.info("Flow Launch: {}", flowLaunch);

        var accessor = new FlowResourcesAccessor(calculator);
        var ref = accessor.collectJobResources(flowLaunch, "sandbox_release_states_testing");

        var resources = resourceService.loadResources(ref);
        for (var resource : resources.getAll()) {
            log.info("{}", resource.getData());
        }
    }

    public static void main(String[] args) {
        startAndStopThisClass(args, Environment.STABLE);
    }
}
