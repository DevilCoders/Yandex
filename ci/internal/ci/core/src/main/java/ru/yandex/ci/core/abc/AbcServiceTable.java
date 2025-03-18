package ru.yandex.ci.core.abc;

import java.util.Collection;
import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;

import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.repository.db.Table;
import yandex.cloud.repository.kikimr.table.KikimrTable;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import yandex.cloud.repository.kikimr.yql.YqlView;
import ru.yandex.ci.common.ydb.KikimrTableCi;

public class AbcServiceTable extends KikimrTableCi<AbcServiceEntity> {

    public AbcServiceTable(KikimrTable.QueryExecutor executor) {
        super(AbcServiceEntity.class, executor);
    }

    public List<AbcServiceEntity> loadServices(Collection<String> slugs) {
        return this.find(
                YqlPredicate.where("slug").in(slugs),
                YqlView.index(AbcServiceEntity.IDX_BY_SLUG),
                YqlOrderBy.orderBy("slug")
        );
    }

    public Set<String> findServiceSlugs(String filter) {
        var slugViews = this.find(SlugView.class,
                YqlPredicate.where("slug").like(filter)
                        .or(YqlPredicate.where("nameLower.en").like(filter)),
                YqlOrderBy.orderBy("slug"));

        return slugViews.stream()
                .map(SlugView::getSlug)
                .collect(Collectors.toSet());
    }

    @Value
    static class SlugView implements Table.View {

        @Column
        String slug;
    }
}
