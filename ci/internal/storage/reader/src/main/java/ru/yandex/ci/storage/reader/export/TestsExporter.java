package ru.yandex.ci.storage.reader.export;

import java.util.List;
import java.util.stream.Collectors;

import ru.yandex.ci.storage.reader.cache.ReaderCache;
import ru.yandex.ci.storage.reader.message.main.CheckTaskMessage;

public class TestsExporter {
    private final ExportStreamWriter exportStreamWriter;
    // We can not fully rely on cache as api and other readers can make modifications to db.
    // We can rely on cache for task statistics, as it divided by hosts in db.
    private final ReaderCache readerCache;

    public TestsExporter(ExportStreamWriter exportStreamWriter, ReaderCache readerCache) {
        this.exportStreamWriter = exportStreamWriter;
        this.readerCache = readerCache;
    }

    public void exportMessageAutocheckTestResults(List<CheckTaskMessage> messages) {
        var resultsWithTaskInfo = messages.stream()
                .flatMap(m -> m.getMessage().getAutocheckTestResults().getResultsList().stream()
                        .map(r -> {
                            var taskId = m.getCheckTaskId();
                            var check = readerCache.checks().getOrThrow(taskId.getIterationId().getCheckId());
                            var task = readerCache.checkTasks().getOrThrow(taskId);

                            return new CheckTaskTestResult(check, task, m.getMessage().getPartition(), r);
                        })
                )
                .collect(Collectors.toList());

        exportStreamWriter.onAutocheckTestResults(resultsWithTaskInfo);
    }
}
