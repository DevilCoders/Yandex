package ru.yandex.ci.storage.reader.message.writer;

import java.util.List;

import ru.yandex.ci.storage.core.ShardIn;
import ru.yandex.ci.storage.reader.cache.ReaderCache;

public interface ShardInMessageWriter {
    void writeChunkMessages(ReaderCache.Modifiable cache, List<ShardIn.ChunkMessage> messages);
}
