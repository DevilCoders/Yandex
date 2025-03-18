package ru.yandex.ci.storage.core.db.model.test_status;

import javax.annotation.Nullable;

import lombok.Value;

import ru.yandex.ci.storage.core.db.model.revision.RevisionEntity;
import ru.yandex.ci.storage.core.db.model.test_mute.TestMuteEntity;
import ru.yandex.ci.storage.core.db.model.test_statistics.TestStatisticsEntity;

@Value
public class TestStatusWithStatistics {
    TestStatusEntity status;
    TestStatisticsEntity statistics;
    RevisionEntity revision;

    @Nullable
    TestMuteEntity mute;
}
