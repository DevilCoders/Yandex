package ru.yandex.ci.storage.reader.export;

import java.util.List;

import lombok.extern.slf4j.Slf4j;

@Slf4j
public class ExportStreamWriterEmptyImpl implements ExportStreamWriter {
    @Override
    public void onAutocheckTestResults(List<CheckTaskTestResult> testResults) {
        log.info("Received for export stream write {} test results, no ops in empty writer", testResults.size());
    }
}
