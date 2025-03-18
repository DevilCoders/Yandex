package ru.yandex.ci.core.arc.util;

import java.util.ArrayList;
import java.util.List;
import java.util.function.Supplier;

import com.google.common.base.Suppliers;

import ru.yandex.arc.api.Repo.ChangelistResponse.ChangeType;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.CommitId;

public class CommitPathFetcherMemoized implements Supplier<List<String>> {

    private final Supplier<List<String>> supplier;

    public CommitPathFetcherMemoized(ArcService arcService, CommitId revision) {
        supplier = Suppliers.memoize(() -> {
            var paths = new ArrayList<String>();
            arcService.processChanges(revision, null, change -> {
                paths.add(change.getPath());
                if (change.hasSource() && change.getChange() == ChangeType.Move) {
                    paths.add(change.getSource().getPath());
                }
            });
            return paths;
        });
    }

    @Override
    public List<String> get() {
        return supplier.get();
    }

}
