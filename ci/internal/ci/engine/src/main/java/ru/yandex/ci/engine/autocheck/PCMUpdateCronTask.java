package ru.yandex.ci.engine.autocheck;

import java.time.Duration;

import lombok.extern.slf4j.Slf4j;
import org.apache.curator.framework.CuratorFramework;

import ru.yandex.ci.engine.common.CiEngineCronTask;
import ru.yandex.ci.engine.pcm.PCMService;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.lang.NonNullApi;

@Slf4j
@NonNullApi
public class PCMUpdateCronTask extends CiEngineCronTask {
    private final PCMService pcmService;

    public PCMUpdateCronTask(PCMService pcmService, Duration runDelay, Duration timeout, CuratorFramework curator) {
        super(runDelay, timeout, curator);
        this.pcmService = pcmService;
    }

    @Override
    public void executeImpl(ExecutionContext executionContext) throws Exception {
        log.info("Trying to update pool nodes in PCM service");
        pcmService.updatePools();
    }
}
