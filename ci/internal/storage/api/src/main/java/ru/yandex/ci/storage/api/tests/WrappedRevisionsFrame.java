package ru.yandex.ci.storage.api.tests;

import java.util.List;

import lombok.Value;

@Value
public class WrappedRevisionsFrame {
    List<TestHistoryPage.TestRevision> revisions;
}
