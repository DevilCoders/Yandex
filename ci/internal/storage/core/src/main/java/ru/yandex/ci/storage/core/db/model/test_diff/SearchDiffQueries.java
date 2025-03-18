package ru.yandex.ci.storage.core.db.model.test_diff;

import java.util.ArrayList;
import java.util.Optional;
import java.util.Set;

import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;

import ru.yandex.ci.common.ydb.YqlPredicateCi;
import ru.yandex.ci.storage.api.StorageFrontApi;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.constant.ResultTypeUtils;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.utils.TextFilter;
import ru.yandex.ci.util.ObjectStore;

public class SearchDiffQueries {
    private SearchDiffQueries() {

    }

    public static ObjectStore<YqlPredicate> getSearchSuitesFilterNotAllToolchain(
            CheckIterationEntity.Id id,
            SuiteSearchFilters filters
    ) {
        final var filter = new ObjectStore<>(
                YqlPredicate
                        .where("id.checkId").eq(id.getCheckId())
                        .and("id.iterationType").eq(id.getIterationType().getNumber())
                        .and("id.toolchain").neq(TestEntity.ALL_TOOLCHAINS)
        );

        return getSearchSuitesFilter(id, filters, filter);
    }

    public static ObjectStore<YqlPredicate> getSearchSuitesFilter(
            CheckIterationEntity.Id id,
            SuiteSearchFilters filters
    ) {
        final var filter = new ObjectStore<>(
                YqlPredicate
                        .where("id.checkId").eq(id.getCheckId())
                        .and("id.iterationType").eq(id.getIterationType().getNumber())
                        .and("id.toolchain").eq(filters.getToolchain())
        );

        return getSearchSuitesFilter(id, filters, filter);
    }

    private static ObjectStore<YqlPredicate> getSearchSuitesFilter(
            CheckIterationEntity.Id id,
            SuiteSearchFilters filters,
            ObjectStore<YqlPredicate> filter
    ) {
        fillIterationFilter(id, filter);

        var resultType = filters.getResultTypes();
        fillResultTypesFilter(filter, resultType.isEmpty() ? ResultTypeUtils.NOT_CHILD_TYPE : resultType);
        fillCommonFiltersWithPath(filters, filter);

        return filter;
    }

    public static YqlOrderBy.SortOrder fillPageFilters(
            ObjectStore<YqlPredicate> filter, SuitePageCursor diffPageCursor
    ) {
        if (diffPageCursor.getForward() != null) {
            filter.set(
                    filter.get().and(
                            YqlPredicate
                                    .where("id.path").eq(diffPageCursor.getForward().getPath())
                                    .and("id.suiteId").gte(diffPageCursor.getForward().getSuiteId())
                                    .or("id.path").gt(diffPageCursor.getForward().getPath())
                    )

            );
            return YqlOrderBy.SortOrder.ASC;
        } else if (diffPageCursor.getBackward() != null) {
            filter.set(
                    filter.get().and(
                            YqlPredicate
                                    .where("id.path").eq(diffPageCursor.getBackward().getPath())
                                    .and("id.suiteId").lt(diffPageCursor.getBackward().getSuiteId())
                                    .or("id.path").lt(diffPageCursor.getBackward().getPath())
                    )
            );
            return YqlOrderBy.SortOrder.DESC;
        }

        return YqlOrderBy.SortOrder.ASC;
    }

    public static void fillIterationFilter(CheckIterationEntity.Id id, ObjectStore<YqlPredicate> filter) {
        if (id.getNumber() == 0) {
            filter.set(filter.get().and(YqlPredicate.where("isLast").eq(true)));
        } else {
            filter.set(filter.get().and(YqlPredicate.where("id.iterationNumber").eq(id.getNumber())));
        }
    }

    public static void fillResultTypesFilter(
            ObjectStore<YqlPredicate> filter,
            Set<Common.ResultType> resultTypes
    ) {
        if (!resultTypes.isEmpty()) {
            if (resultTypes.size() == 1) {
                filter.set(filter.get().and("id.resultType").eq(resultTypes.iterator().next()));
            } else {
                filter.set(filter.get().and(YqlPredicateCi.in("id.resultType", resultTypes)));
            }
        }
    }

    public static void fillCommonFiltersWithPath(SuiteSearchFilters filters, ObjectStore<YqlPredicate> filter) {
        fillCommonFilters(filters, filter);

        if (!filters.getPath().isEmpty()) {
            addTextFilter(filter, "id.path", filters.getPath());
        }
    }

    public static void fillCommonFilters(SuiteSearchFilters filters, ObjectStore<YqlPredicate> filter) {
        var status = filters.getSpecialCases().equals(StorageFrontApi.SpecialCasesFilter.SPECIAL_CASE_BROKEN_BY_DEPS) ?
                StorageFrontApi.StatusFilter.STATUS_FAILED_WITH_DEPS : filters.getStatus();

        getSpecialCasesPredicate(filters).ifPresentOrElse(
                predicate -> filter.set(filter.get().and(predicate)),
                () -> {
                    getStatusPredicate(status).ifPresent(predicate -> filter.set(filter.get().and(predicate)));

                    if (filters.getCategory().equals(StorageFrontApi.CategoryFilter.CATEGORY_CHANGED)) {
                        filter.set(filter.get().and(getChangedPredicate(status)));
                    }
                }
        );
    }

    public static void addTextFilter(ObjectStore<YqlPredicate> filter, String field, String value) {
        filter.set(filter.get().and(TextFilter.get(field, value)));
    }

    public static YqlPredicate getChangedPredicate(StorageFrontApi.StatusFilter status) {
        return switch (status) {
            case STATUS_ALL, UNRECOGNIZED -> YqlPredicate
                    .where("statistics.self.stage.passedAdded").gt(0)
                    .or("statistics.self.stage.failedAdded").gt(0)
                    .or("statistics.self.stage.skippedAdded").gt(0)
                    .or("statistics.children.stage.passedAdded").gt(0)
                    .or("statistics.children.stage.failedAdded").gt(0)
                    .or("statistics.children.stage.skippedAdded").gt(0);
            case STATUS_PASSED -> YqlPredicate
                    .where("statistics.self.stage.passedAdded").gt(0)
                    .or("statistics.children.stage.passedAdded").gt(0);
            case STATUS_FAILED -> YqlPredicate
                    .where("statistics.self.stage.failedAdded").gt(0)
                    .or("statistics.children.stage.failedAdded").gt(0);
            case STATUS_FAILED_WITH_DEPS -> YqlPredicate
                    .where("statistics.self.stage.failedAdded").gt(0)
                    .or("statistics.children.stage.failedAdded").gt(0)
                    .or("statistics.children.stage.failedByDepsAdded").gt(0)
                    .or("statistics.children.stage.failedByDepsAdded").gt(0);
            case STATUS_SKIPPED -> YqlPredicate
                    .where("statistics.self.stage.skippedAdded").gt(0)
                    .or("statistics.children.stage.skippedAdded").gt(0);
        };
    }

    public static Optional<YqlPredicate> getStatusSelfPredicate(StorageFrontApi.StatusFilter status) {
        return switch (status) {
            case STATUS_ALL -> Optional.empty();
            case STATUS_FAILED -> Optional.of(
                    YqlPredicate.where("statistics.self.stage.failed").gt(0)
            );
            case STATUS_FAILED_WITH_DEPS -> Optional.of(
                    YqlPredicate
                            .where("statistics.self.stage.failed").gt(0)
                            .or("statistics.self.stage.failedByDeps").gt(0)
            );
            case STATUS_PASSED -> Optional.of(
                    YqlPredicate.where("statistics.self.stage.passed").gt(0)
            );
            case STATUS_SKIPPED -> Optional.of(
                    YqlPredicate.where("statistics.self.stage.skipped").gt(0)
            );
            case UNRECOGNIZED -> throw new RuntimeException();
        };
    }

    public static Optional<YqlPredicate> getStatusPredicate(StorageFrontApi.StatusFilter status) {
        var selfPredicate = getStatusSelfPredicate(status);
        if (selfPredicate.isEmpty()) {
            return selfPredicate;
        }

        return switch (status) {
            case STATUS_ALL -> Optional.empty();
            case STATUS_FAILED -> Optional.of(selfPredicate.get().or("statistics.children.stage.failed").gt(0));
            case STATUS_PASSED -> Optional.of(selfPredicate.get().or("statistics.children.stage.passed").gt(0));
            case STATUS_FAILED_WITH_DEPS -> Optional.of(
                    selfPredicate.get().or("statistics.children.stage.failedByDeps").gt(0)
            );
            case STATUS_SKIPPED -> Optional.of(selfPredicate.get().or("statistics.children.stage.skipped").gt(0));
            case UNRECOGNIZED -> throw new RuntimeException();
        };
    }

    @SuppressWarnings("ConstantConditions")
    public static Optional<YqlPredicate> getSpecialCasesPredicate(SuiteSearchFilters filters) {
        if (filters.getSpecialCases().equals(StorageFrontApi.SpecialCasesFilter.SPECIAL_CASE_NONE)) {
            return Optional.empty();
        }

        var category = filters.getCategory();

        var prefix = "statistics." + SpecialCaseFields.TYPE + ".";

        var fields = switch (filters.getSpecialCases()) {
            case SPECIAL_CASE_EXTERNAL -> new SpecialCaseFields(
                    prefix + "extended.external.",
                    SpecialCaseFields.TOTAL, SpecialCaseFields.PASSED_ADDED, SpecialCaseFields.FAILED_ADDED
            );
            case SPECIAL_CASE_INTERNAL -> new SpecialCaseFields(
                    prefix + "extended.internal.",
                    SpecialCaseFields.TOTAL, SpecialCaseFields.PASSED_ADDED, SpecialCaseFields.FAILED_ADDED
            );
            case SPECIAL_CASE_TIMEOUT -> new SpecialCaseFields(
                    prefix + "extended.timeout.",
                    SpecialCaseFields.TOTAL, SpecialCaseFields.PASSED_ADDED, SpecialCaseFields.FAILED_ADDED
            );
            case SPECIAL_CASE_MUTED -> new SpecialCaseFields(
                    prefix + "extended.muted.",
                    SpecialCaseFields.TOTAL, SpecialCaseFields.PASSED_ADDED, SpecialCaseFields.FAILED_ADDED
            );
            case SPECIAL_CASE_FLAKY -> new SpecialCaseFields(
                    prefix + "extended.flaky.",
                    SpecialCaseFields.TOTAL, SpecialCaseFields.PASSED_ADDED, SpecialCaseFields.FAILED_ADDED
            );
            case SPECIAL_CASE_ADDED -> new SpecialCaseFields(
                    prefix + "extended.added.",
                    SpecialCaseFields.TOTAL, SpecialCaseFields.PASSED_ADDED, SpecialCaseFields.FAILED_ADDED
            );
            case SPECIAL_CASE_DELETED -> new SpecialCaseFields(
                    prefix + "extended.deleted.",
                    SpecialCaseFields.TOTAL, SpecialCaseFields.PASSED_ADDED, SpecialCaseFields.FAILED_ADDED
            );
            case SPECIAL_CASE_BROKEN_BY_DEPS -> new SpecialCaseFields(
                    prefix + "stage.", "failedByDeps", "passedByDepsAdded", "failedByDepsAdded"
            );
            case SPECIAL_CASE_FAILED_IN_STRONG_MODE -> new SpecialCaseFields(
                    prefix + "stage.", "failedInStrongMode", "failedInStrongMode", "failedInStrongMode"
            );
            case UNRECOGNIZED, SPECIAL_CASE_NONE -> throw new RuntimeException();
        };

        var status = filters.getStatus();
        if (category.equals(StorageFrontApi.CategoryFilter.CATEGORY_CHANGED)) {
            return switch (status) {
                case STATUS_ALL -> Optional.of(
                        YqlPredicate
                                .where(fields.getSelfFailedAdded()).gt(0)
                                .or(fields.getSelfPassedAdded()).gt(0)
                                .or(fields.getChildrenFailedAdded()).gt(0)
                                .or(fields.getChildrenPassedAdded()).gt(0)
                );
                case STATUS_FAILED, STATUS_FAILED_WITH_DEPS -> Optional.of(
                        YqlPredicate
                                .where(fields.getSelfFailedAdded()).gt(0)
                                .or(fields.getChildrenFailedAdded()).gt(0)
                );
                case STATUS_PASSED -> Optional.of(
                        YqlPredicate
                                .where(fields.getSelfPassedAdded()).gt(0)
                                .or(fields.getChildrenPassedAdded()).gt(0)
                );
                case STATUS_SKIPPED, UNRECOGNIZED -> Optional.of(YqlPredicate.alwaysFalse());
            };
        }

        if (!status.equals(StorageFrontApi.StatusFilter.STATUS_ALL)) {
            // Not supported because we have only total and changed errors.
            return Optional.of(YqlPredicate.alwaysFalse());
        }

        return Optional.of(
                YqlPredicate
                        .where(fields.getSelfTotal()).gt(0)
                        .or(fields.getChildrenTotal()).gt(0)
        );
    }

    public static ObjectStore<YqlPredicate> getSearchDiffsFilter(
            CheckIterationEntity.Id iterationId, DiffSearchFilters filters
    ) {
        final var filter = new ObjectStore<>(
                YqlPredicate.
                        where("id.checkId").eq(iterationId.getCheckId())
                        .and("id.iterationType").eq(iterationId.getIterationType().getNumber())
        );

        SearchDiffQueries.fillIterationFilter(iterationId, filter);
        SearchDiffQueries.fillResultTypesFilter(filter, filters.getResultTypes());

        fillNotAggregateToolchainFilter(filter, filters.getToolchain());

        var diffTypes = filters.getDiffTypes();
        if (!diffTypes.isEmpty()) {
            if (diffTypes.size() == 1) {
                filter.set(filter.get().and("diffType").eq(diffTypes.iterator().next()));
            } else {
                filter.set(filter.get().and(YqlPredicateCi.in("diffType", diffTypes)));
            }
        }

        if (!filters.getPath().isEmpty()) {
            SearchDiffQueries.addTextFilter(filter, "id.path", filters.getPath());
        }

        if (!filters.getPathIn().isEmpty()) {
            filter.set(filter.get().and("id.path").in(filters.getPathIn()));
        }

        if (!filters.getName().isEmpty()) {
            SearchDiffQueries.addTextFilter(filter, "name", filters.getName());
        }

        if (!filters.getSubtestName().isEmpty()) {
            SearchDiffQueries.addTextFilter(filter, "subtestName", filters.getSubtestName());
        }

        if (
                !filters.getNotificationFilter().equals(StorageFrontApi.NotificationFilter.NF_NONE) &&
                        !filters.getNotificationFilter().equals(StorageFrontApi.NotificationFilter.UNRECOGNIZED)
        ) {
            filter.set(filter.get().and("isMuted").eq(
                    filters.getNotificationFilter().equals(StorageFrontApi.NotificationFilter.NF_MUTED))
            );
        }

        if (!filters.getTags().isEmpty()) {
            var tagsPredicates = new ArrayList<YqlPredicate>(filters.getTags().size());
            for (var tag : filters.getTags()) {
                tagsPredicates.add(YqlPredicate.where("tags").like("%|" + tag + "|%"));
            }

            filter.set(filter.get().and(YqlPredicate.and(tagsPredicates)));
        }

        return filter;
    }

    public static void fillNotAggregateToolchainFilter(ObjectStore<YqlPredicate> filter, String toolchain) {
        if (toolchain.isEmpty() || toolchain.equals(TestEntity.ALL_TOOLCHAINS)) {
            filter.set(filter.get().and("id.toolchain").neq(TestEntity.ALL_TOOLCHAINS));
        } else {
            filter.set(filter.get().and("id.toolchain").eq(toolchain));
        }
    }

    private static class SpecialCaseFields {
        private static final String TYPE = "{type}";
        private static final String TOTAL = "total";
        private static final String PASSED_ADDED = "passedAdded";
        private static final String FAILED_ADDED = "failedAdded";

        private final String total;
        private final String passedAdded;
        private final String failedAdded;

        private SpecialCaseFields(String prefix, String total, String passedAdded, String failedAdded) {
            this.total = prefix + total;
            this.passedAdded = prefix + passedAdded;
            this.failedAdded = prefix + failedAdded;
        }

        public String getSelfTotal() {
            return total.replace(TYPE, "self");
        }

        public String getSelfPassedAdded() {
            return passedAdded.replace(TYPE, "self");
        }

        public String getSelfFailedAdded() {
            return failedAdded.replace(TYPE, "self");
        }

        public String getChildrenTotal() {
            return total.replace(TYPE, "children");
        }

        public String getChildrenPassedAdded() {
            return passedAdded.replace(TYPE, "children");
        }

        public String getChildrenFailedAdded() {
            return failedAdded.replace(TYPE, "children");
        }
    }
}
