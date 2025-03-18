package ru.yandex.ci.storage.reader.export;

import java.util.List;

public interface ExportStreamWriter {
    void onAutocheckTestResults(List<CheckTaskTestResult> testResults);
}
