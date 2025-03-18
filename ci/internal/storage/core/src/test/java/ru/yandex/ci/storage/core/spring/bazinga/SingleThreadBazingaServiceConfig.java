package ru.yandex.ci.storage.core.spring.bazinga;

import org.springframework.context.ApplicationContext;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.common.bazinga.spring.BazingaCoreConfig;
import ru.yandex.ci.storage.core.bazinga.SingleThreadBazinga;
import ru.yandex.commune.bazinga.context.BazingaWorkerTaskInitializer;
import ru.yandex.commune.bazinga.impl.storage.BazingaStorage;
import ru.yandex.commune.bazinga.impl.worker.WorkerTaskRegistry;

@Configuration
@Import(BazingaCoreConfig.class)
public class SingleThreadBazingaServiceConfig {

    @Bean
    public SingleThreadBazinga singleThreadBazinga(
            BazingaStorage storage,
            WorkerTaskRegistry workerTaskRegistry
    ) {
        return new SingleThreadBazinga(storage, workerTaskRegistry);
    }

    @Bean
    public WorkerTaskRegistry workerTaskRegistry() {
        return new WorkerTaskRegistry();
    }

    @Bean
    public BazingaWorkerTaskInitializer bazingaWorkerTaskInitializer(
            WorkerTaskRegistry workerTaskRegistry,
            ApplicationContext applicationContext
    ) {
        return new BazingaWorkerTaskInitializer(
                applicationContext,
                workerTaskRegistry
        );
    }
}
