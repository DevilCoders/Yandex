package ru.yandex.ci.engine.discovery;

import java.util.Collection;
import java.util.List;
import java.util.Set;
import java.util.function.Predicate;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.abc.AbcService;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommitUtils;
import ru.yandex.ci.core.config.a.model.FilterConfig;
import ru.yandex.ci.engine.discovery.filter.PathFilterProcessor;
import ru.yandex.ci.util.GlobMatchers;

@Slf4j
@RequiredArgsConstructor
public class DiscoveryServiceFilters {

    @Nonnull
    private final AbcService abcService;

    DiscoveryModesFilter getFilter(DiscoveryContext context) {
        return new DiscoveryModesFilter(context);
    }

    @AllArgsConstructor(access = AccessLevel.PRIVATE)
    class DiscoveryModesFilter {

        @Nonnull
        private final DiscoveryContext context;

        List<FilterConfig> findSuitableFilters(List<FilterConfig> filters) {
            var suitableFilters = filters;

            if (suitableFilters.isEmpty()) {
                suitableFilters = switch (context.getDiscoveryType()) {
                    case DIR -> List.of(FilterConfig.defaultFilter());
                    case GRAPH, STORAGE, PCI_DSS -> {
                        log.info("Empty filter filtered commit cause of inappropriate discovery mode. Got {}",
                                context.getDiscoveryType());
                        yield List.of();
                    }
                };
            }

            suitableFilters = suitableFilters.stream().filter(context.getFilterPredicate()).toList();

            if (suitableFilters.isEmpty()) {
                log.info("No suitable filters found");
                return List.of();
            }

            return suitableFilters.stream()
                    .filter(this::doesCommitBelongsToCiProcessId)
                    .collect(Collectors.toList());
        }

        private boolean doesCommitBelongsToCiProcessId(FilterConfig filter) {
            var commitAuthor = context.getCommit().getAuthor();
            if (!isCorrectAuthor(commitAuthor, filter)) {
                log.info(
                        "Filter {} filtered commit cause of inappropriate author: {}",
                        filter, commitAuthor
                );
                return false;
            }

            var commitMessage = context.getCommit().getMessage();
            if (!isCorrectStQueue(commitMessage, filter)) {
                log.info(
                        "Filter {} filtered commit cause of inappropriate ST queue. Commit message: {}",
                        filter, commitMessage
                );
                return false;
            }

            if (!PathFilterProcessor.hasCorrectPaths(context, filter)) {
                return false;
            }

            var arcBranch = context.getFeatureBranch();
            if (!isAcceptedBranchNames(arcBranch, filter)) {
                log.info(
                        "Filter {} filtered commit cause of inappropriate branch name: {}",
                        filter, arcBranch
                );
                return false;
            }
            return true;
        }
    }


    private boolean isCorrectAuthor(String author, FilterConfig filter) {
        if (filter.getNotAuthors().contains(author)) {
            return false;
        }

        return valueMeetsCriteria(
                filter.getAuthorServices(),
                filter.getNotAuthorServices(),
                service -> abcService.isMember(author, service)
        );
    }

    static boolean isCorrectStQueue(String commitMessage, FilterConfig filter) {
        if (filter.getStQueues().isEmpty() && filter.getNotStQueues().isEmpty()) {
            return true;
        }
        Set<String> commitStQueues = ArcCommitUtils.parseStQueues(commitMessage);
        return valueMeetsCriteria(
                filter.getStQueues(),
                filter.getNotStQueues(),
                commitStQueues::contains
        );
    }

    static boolean isAcceptedBranchNames(@Nullable ArcBranch branch, FilterConfig filter) {
        if (filter.getFeatureBranches().isEmpty() && filter.getNotFeatureBranches().isEmpty()) {
            return true;
        }
        if (branch == null) {
            return false;
        }

        return GlobMatchers.pathsMatchGlobs(
                List.of(branch.asString()),
                filter.getFeatureBranches(),
                filter.getNotFeatureBranches()
        );
    }

    static <T> boolean valueMeetsCriteria(
            Collection<T> positiveSamples, Collection<T> negativeSamples, Predicate<T> tester) {
        return valueMeetsCriteria(positiveSamples, tester, negativeSamples, tester);
    }

    private static <T> boolean valueMeetsCriteria(
            Collection<T> positiveSamples,
            Predicate<T> positiveTester,
            Collection<T> negativeSamples,
            Predicate<T> negativeTester
    ) {
        var nonePositiveMatches = positiveSamples.stream().noneMatch(positiveTester);
        if (!positiveSamples.isEmpty() && nonePositiveMatches) {
            return false;
        }
        return negativeSamples.stream().noneMatch(negativeTester);
    }
}
