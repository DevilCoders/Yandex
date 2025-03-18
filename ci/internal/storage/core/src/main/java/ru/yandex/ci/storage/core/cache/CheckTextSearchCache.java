package ru.yandex.ci.storage.core.cache;

import java.util.List;

import ru.yandex.ci.storage.core.db.model.check_text_search.CheckTextSearchEntity;

public interface CheckTextSearchCache extends StorageCustomCache {
    boolean contains(CheckTextSearchEntity.Id id);

    void put(List<CheckTextSearchEntity> values);
}
