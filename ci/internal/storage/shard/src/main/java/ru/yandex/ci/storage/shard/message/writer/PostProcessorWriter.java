package ru.yandex.ci.storage.shard.message.writer;

import java.util.List;

import ru.yandex.ci.storage.core.PostProcessor;

public interface PostProcessorWriter {
    void writeResults(List<PostProcessor.ResultForPostProcessor> results);
}
