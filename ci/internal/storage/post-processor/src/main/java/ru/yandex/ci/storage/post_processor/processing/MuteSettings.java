package ru.yandex.ci.storage.post_processor.processing;

import lombok.Value;

@Value
public class MuteSettings {
    int daysWithoutFlaky;
    // OR
    int numberOfNonFlakyRuns;
}
