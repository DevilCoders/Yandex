package ru.yandex.ci.core.arc;

import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;
import java.util.function.Function;
import java.util.function.Predicate;
import java.util.stream.Stream;

import com.google.common.base.Preconditions;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.scheduling.annotation.Scheduled;

import ru.yandex.ci.util.ExceptionUtils;

@Slf4j
@RequiredArgsConstructor
public class ArcBranchCache {

    private final AtomicReference<List<String>> branches = new AtomicReference<>();
    private final CountDownLatch branchesLoaded = new CountDownLatch(1);
    private final ArcService arcService;

    @Scheduled(
            fixedDelayString = "${ci.arcBranchCache.refreshDelaySeconds}",
            initialDelay = 0,
            timeUnit = TimeUnit.SECONDS
    )
    public void updateCache() {
        log.info("Refreshing branches...");
        var time = System.currentTimeMillis();
        var list = arcService.getBranches("");
        log.info("Loaded total branches: {} in {} msec", list.size(), System.currentTimeMillis() - time);
        this.branches.set(list);
        this.branchesLoaded.countDown();
    }

    public Stream<String> suggestBranches(String branch) {
        try {
            var branchList = getLoadedCache();
            return Stream
                    .of(
                            getBranchByName(branchList, branch),
                            getBranchesByPrefix(branchList, branch),
                            getBranchesBySubstring(branchList, branch)
                    )
                    .flatMap(Function.identity())
                    .distinct();
        } catch (Exception e) {
            throw ExceptionUtils.unwrap(e);
        }
    }

    private List<String> getLoadedCache() throws InterruptedException {
        this.branchesLoaded.await();
        var cache = this.branches.get();
        Preconditions.checkState(cache != null, "Internal error, cache cannot be null");
        return cache;
    }

    static Stream<String> getBranchesBySubstring(List<String> branches, String substring) {
        return filter(branches, name -> name.contains(substring));
    }

    static Stream<String> getBranchByName(List<String> branches, String branch) {
        return filter(branches, name -> name.equals(branch));
    }

    static Stream<String> getBranchesByPrefix(List<String> branches, String prefix) {
        return filter(branches, name -> name.startsWith(prefix));
    }

    private static Stream<String> filter(List<String> branches, Predicate<String> predicate) {
        return branches.stream().filter(predicate);
    }
}
