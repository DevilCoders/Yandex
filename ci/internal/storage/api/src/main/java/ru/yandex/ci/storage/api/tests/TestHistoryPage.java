package ru.yandex.ci.storage.api.tests;

import java.util.List;
import java.util.Map;
import java.util.function.Function;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import lombok.Value;

import ru.yandex.ci.storage.core.db.model.revision.RevisionEntity;
import ru.yandex.ci.storage.core.db.model.test_revision.HistoryPageCursor;
import ru.yandex.ci.storage.core.db.model.test_revision.TestRevisionEntity;
import ru.yandex.ci.storage.core.db.model.test_revision.WrappedRevisionsBoundaries;

@Value
public class TestHistoryPage {
    List<TestRevision> revisions;
    HistoryPageCursor paging;

    @Value
    public static class TestRevision {
        RevisionEntity revision;
        Map<String, TestRevisionEntity> toolchains;

        @Nullable
        WrappedRevisionsBoundaries wrappedRevisionsBoundaries;

        public TestRevision(
                RevisionEntity revision,
                List<TestRevisionEntity> toolchains,
                WrappedRevisionsBoundaries wrappedRevisionsBoundaries
        ) {
            this.revision = revision;
            this.toolchains = toolchains.stream().collect(
                    Collectors.toMap(x -> x.getId().getStatusId().getToolchain(), Function.identity())
            );

            this.wrappedRevisionsBoundaries = wrappedRevisionsBoundaries;
        }

        public TestRevision(TestRevision old, WrappedRevisionsBoundaries wrappedRevisionsBoundaries) {
            this.revision = old.revision;
            this.toolchains = old.toolchains;
            this.wrappedRevisionsBoundaries = wrappedRevisionsBoundaries;
        }
    }
}
