package ru.yandex.ci.engine.pcm;

import java.util.List;

import ru.yandex.ci.core.db.autocheck.model.AccessControl;
import ru.yandex.lang.NonNullApi;

@NonNullApi
public interface PCMService {
    void updatePools();

    List<PoolNode> getAvailablePools(List<AccessControl> acIds);
}
