package ru.yandex.ci.storage.core.db.model.task_result;

import java.util.Collections;
import java.util.List;

import lombok.Value;

import ru.yandex.ci.ydb.Persisted;
import ru.yandex.inside.yt.kosher.impl.ytree.object.annotation.YTreeObject;

@Persisted
@YTreeObject
@Value
public class ResultOwners {
    public static final ResultOwners EMPTY = new ResultOwners(Collections.emptyList(), Collections.emptyList());

    List<String> logins;
    List<String> groups;
}
