package ru.yandex.ci.common.bazinga.spring;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.commune.bazinga.impl.worker.WorkerTaskLogger;
import ru.yandex.commune.bazinga.impl.worker.WorkerTaskLoggerSource;

@Configuration
@Import(S3LogStorageTestConfig.class)
public class S3BazingaLoggerTestConfig {

    @Bean
    public WorkerTaskLoggerSource workerTaskLoggerSource() {
        return logDirManager ->
                (WorkerTaskLogger) (task, runWithLog) ->
                        runWithLog.accept(null); // Do not save anything into separate files
    }

}
