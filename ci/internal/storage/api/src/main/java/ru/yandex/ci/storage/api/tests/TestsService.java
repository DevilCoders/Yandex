package ru.yandex.ci.storage.api.tests;

import java.util.Comparator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

import lombok.AllArgsConstructor;

import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.test_status.TestSearch;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;

@AllArgsConstructor
public class TestsService {
    private final CiStorageDb db;

    public TestSearch.Results search(TestSearch search) {
        /* optimized
        return db.readOnly().withStatementIsolationLevel(IsolationLevel.STALE_CONSISTENT_READ_ONLY).run(
                () -> this.searchInTx(search)
        );
         */

        return db.scan().run(() -> this.searchInTx(search));
    }

    private TestSearch.Results searchInTx(TestSearch search) {
        var paging = search.getPaging();
        var pageSize = paging.getPageSize();
        var results = this.db.tests().search(search);

        var forward = (TestSearch.Page) null;
        var backward = (TestSearch.Page) null;

        var next = results.size() > pageSize ? results.values().stream()
                .skip(paging.isAscending() ? pageSize : pageSize - 1)
                .findFirst().orElseThrow().get(0) : null;

        if (paging.isAscending()) {
            if (next != null) {
                forward = toPage(next);
            }
            backward = paging.getPage();
        } else if (!paging.getPage().getPath().isEmpty()) {
            forward = paging.getPage();
            if (next != null) {
                backward = toPage(next);
            }
        } else {
            if (next != null) {
                forward = toPage(next);
            }
        }

        var comparator = Comparator.<Map.Entry<TestStatusEntity.Id, List<TestStatusEntity>>, String>comparing(
                        x -> x.getValue().get(0).getPath()
                )
                .thenComparing(x -> x.getValue().get(0).getName());
        return new TestSearch.Results(
                results.entrySet().stream()
                        .limit(pageSize)
                        .sorted(comparator)
                        .collect(Collectors.toMap(
                                Map.Entry::getKey, Map.Entry::getValue, (a, b) -> a, LinkedHashMap::new
                        )),
                forward,
                backward
        );
    }

    private TestSearch.Page toPage(TestStatusEntity value) {
        return new TestSearch.Page(value.getPath(), value.getId().getTestId());
    }
}
