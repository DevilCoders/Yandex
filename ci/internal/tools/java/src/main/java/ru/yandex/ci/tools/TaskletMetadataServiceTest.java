package ru.yandex.ci.tools;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.spring.TaskletMetadataConfig;
import ru.yandex.ci.core.tasklet.TaskletMetadataService;
import ru.yandex.ci.core.tasklet.TaskletMetadataServiceImpl;
import ru.yandex.ci.flow.spring.ydb.YdbCiConfig;

@Import({
        YdbCiConfig.class,
        TaskletMetadataConfig.class
})
@Configuration
public class TaskletMetadataServiceTest extends AbstractSpringBasedApp {

    @Autowired
    private TaskletMetadataService taskletMetadataService;

    @Override
    protected void run() {
        ((TaskletMetadataServiceImpl) taskletMetadataService).fetchFromSandbox(2630585897L, "upload_rps_limits");
    }

    public static void main(String[] args) {
        startAndStopThisClass(args);
    }
}
