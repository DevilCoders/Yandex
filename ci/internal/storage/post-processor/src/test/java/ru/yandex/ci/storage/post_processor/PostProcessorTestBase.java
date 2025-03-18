package ru.yandex.ci.storage.post_processor;

import java.util.Collection;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.stream.Collectors;

import org.junit.jupiter.api.BeforeEach;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.ayamler.Ayamler;
import ru.yandex.ci.client.ayamler.AYamlerClient;
import ru.yandex.ci.client.ayamler.StrongModeRequest;
import ru.yandex.ci.storage.core.StorageYdbTestBase;
import ru.yandex.ci.storage.core.cache.StorageCoreCache;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.utils.TimeTraceService;
import ru.yandex.ci.storage.post_processor.cache.PostProcessorCache;
import ru.yandex.ci.storage.post_processor.spring.StoragePostProcessorTestConfig;

import static org.mockito.ArgumentMatchers.anySet;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.reset;

@ContextConfiguration(classes = StoragePostProcessorTestConfig.class)
public class PostProcessorTestBase extends StorageYdbTestBase {
    protected static final TestEntity.Id TEST_ID = new TestEntity.Id(1L, "b", 3L);

    @Autowired
    protected PostProcessorCache cache;

    @Autowired
    protected AYamlerClient aYamlerClient;

    @Autowired
    protected TimeTraceService timeTraceService;

    @Autowired
    protected PostProcessorStatistics statistics;

    @BeforeEach
    public void clearCache() {
        this.cache.modify(StorageCoreCache.Modifiable::invalidateAll);
    }

    @BeforeEach
    public void mockAYamlerClient() {
        mockAYamlerResponse(Map.of());
    }

    protected void mockAYamlerResponse(Map<String, Ayamler.StrongModeStatus> strongModeByPath) {
        reset(aYamlerClient);
        doAnswer(args -> {
            Collection<StrongModeRequest> strongModeRequests = args.getArgument(0);
            var strongModes = strongModeRequests.stream()
                    .map(request -> toProtoStrongMode(
                            request,
                            strongModeByPath.getOrDefault(request.getPath(), Ayamler.StrongModeStatus.OFF)
                    ))
                    .collect(Collectors.toList());
            return CompletableFuture.completedFuture(
                    Ayamler.GetStrongModeBatchResponse.newBuilder()
                            .addAllStrongMode(strongModes)
                            .build()
            );
        }).when(aYamlerClient).getStrongMode(anySet());
    }

    protected Ayamler.StrongMode toProtoStrongMode(StrongModeRequest request, Ayamler.StrongModeStatus status) {
        return Ayamler.StrongMode.newBuilder()
                .setPath(request.getPath())
                .setRevision(request.getRevision())
                .setStatus(status)
                .setLogin(request.getLogin())
                .build();
    }
}
