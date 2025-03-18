package ru.yandex.ci.storage.core.db.model.task_result;

import lombok.Value;

import ru.yandex.ci.ydb.Persisted;
import ru.yandex.inside.yt.kosher.impl.ytree.object.annotation.YTreeObject;

@Persisted
@YTreeObject
@Value
public class TestOutput {
    String hash;
    String url;
    long size;
}
