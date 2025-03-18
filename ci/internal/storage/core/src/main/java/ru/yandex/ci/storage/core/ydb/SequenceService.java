package ru.yandex.ci.storage.core.ydb;

import java.util.List;

import lombok.AllArgsConstructor;

import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.sequence.DbSequence;

@AllArgsConstructor
public class SequenceService {
    private final CiStorageDb db;

    public void init(DbSequence sequence, long start) {
        this.db.currentOrTx(() -> db.sequences().init(sequence.name(), start));
    }

    public List<Long> next(DbSequence sequence, int amount) {
        return this.db.currentOrTx(() -> db.sequences().incrementAndGet(sequence.name(), amount));
    }
}
