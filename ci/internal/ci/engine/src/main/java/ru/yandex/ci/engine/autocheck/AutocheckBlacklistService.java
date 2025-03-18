package ru.yandex.ci.engine.autocheck;

import java.util.Arrays;
import java.util.List;
import java.util.Set;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;

import com.google.common.annotations.VisibleForTesting;
import com.google.common.cache.CacheBuilder;
import com.google.common.cache.CacheLoader;
import com.google.common.cache.LoadingCache;
import com.google.common.util.concurrent.AbstractScheduledService;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.client.staff.StaffClient;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.autocheck.AutocheckCommit;
import ru.yandex.ci.core.db.CiMainDb;

@Slf4j
public class AutocheckBlacklistService extends AbstractScheduledService {

    public static final String AUTOCHECK_BLACKLIST_PATH = "build/rules/autocheck.blacklist";

    @Nonnull
    private final ArcService arcService;
    @Nonnull
    private final StaffClient staffClient;
    @Nonnull
    private final CiMainDb db;
    @Nonnull
    private final LoadingCache<String, Boolean> robotLoginsStaffCache;
    @Nonnull
    private volatile Set<String> blacklistDirectories;

    public AutocheckBlacklistService(@Nonnull ArcService arcService, @Nonnull StaffClient staffClient,
                                     @Nonnull CiMainDb db) {
        this.arcService = arcService;
        this.staffClient = staffClient;
        this.db = db;
        this.robotLoginsStaffCache = CacheBuilder.newBuilder()
                .recordStats()
                .build(CacheLoader.from(this::isRobotOnStaff));
        this.blacklistDirectories = fetchBlacklistFromArc(arcService);
        log.info("blacklist: {}", this.blacklistDirectories);
    }

    @Override
    protected void runOneIteration() {
        try {
            updateBlacklistDirectories();
        } catch (Exception e) {
            log.error("failed to update blacklist", e);
        }
    }

    @Override
    protected Scheduler scheduler() {
        return Scheduler.newFixedDelaySchedule(5, 5, TimeUnit.MINUTES);
    }

    public boolean skipPostCommit(ArcRevision revision) {
        var autocheckCommitStatus = db.currentOrReadOnly(() -> db.autocheckCommit().find(revision))
                .orElseGet(() -> {
                    var status = shouldCheckPostCommit(revision)
                            ? AutocheckCommit.Status.CHECKING
                            : AutocheckCommit.Status.IGNORED;
                    log.info("Saving revision {}, status {}", revision, status);
                    return db.currentOrTx(() -> db.autocheckCommit().save(revision, status));
                })
                .getStatus();
        return autocheckCommitStatus == AutocheckCommit.Status.IGNORED;
    }

    private boolean shouldCheckPostCommit(ArcRevision revision) {
        var commit = arcService.getCommit(revision);
        /* Conditions were taken from
           https://a.yandex-team.ru/arc_vcs/testenv/core/engine/revision_filter.py?rev=r8780837#L80 */
        if ("arcadia-devtools".equals(commit.getAuthor()) || "robot-ch-sync".equals(commit.getAuthor())) {
            return true;
        }
        if (commit.getMessage().startsWith("Update arcadia_tests_data revisions for")) {
            return true;
        }
        if (commit.getMessage().contains("diff-resolver")) {
            return true;
        }

        var directories = AffectedDirectoriesHelper.collectAndSort(revision, arcService);
        for (var directory : directories) {
            if (directory.startsWith("devtools/dummy_arcadia/simple_review_autobuild_check")) {
                return true;
            }
            if (directory.startsWith("fuzzing")) {
                return true;
            }
        }

        if (isRobot(commit.getAuthor())) {
            log.info("Skip {}: author login {} is robot", revision, commit.getAuthor());
            return false;
        }

        if (isOnlyBlacklistPathsAffected(directories)) {
            log.info("Skip {}: only black list paths affected: {}", revision, blacklistDirectories);
            return false;
        }

        return true;
    }

    public boolean isOnlyBlacklistPathsAffected(ArcRevision arcRevision) {
        var directories = AffectedDirectoriesHelper.collectAndSort(arcRevision, arcService);
        return isOnlyBlacklistPathsAffected(directories);
    }

    public boolean isOnlyBlacklistPathsAffected(List<String> directories) {
        return isOnlyBlacklistPathsAffected(directories, blacklistDirectories);
    }

    static boolean isOnlyBlacklistPathsAffected(List<String> directories, Set<String> blacklistDirectories) {
        if (directories.isEmpty()) {
            return false;
        }
        for (var directory : directories) {
            if (!startsWithAnySubpaths(directory, blacklistDirectories)) {
                return false;
            }
        }
        return true;
    }

    public void updateBlacklistDirectories() {
        log.info("Started updateBlacklistDirectories");
        blacklistDirectories = fetchBlacklistFromArc(arcService);
        log.info("Finished updateBlacklistDirectories, new blacklist: {}", blacklistDirectories);
    }

    @VisibleForTesting
    public void resetCache() {
        robotLoginsStaffCache.invalidateAll();
    }

    private boolean isRobot(String login) {
        try {
            return robotLoginsStaffCache.get(login);
        } catch (ExecutionException e) {
            log.error("Can't get profile from staff: login {}", login, e);
            for (var robotPrefix : List.of("robot-", "zomb-", "search-")) {
                if (login.startsWith(robotPrefix)) {
                    return true;
                }
            }
        }
        return false;
    }

    private boolean isRobotOnStaff(String login) {
        return staffClient.getStaffPerson(login).getOfficial().isRobot();
    }

    private static Set<String> fetchBlacklistFromArc(ArcService arcService) {
        var headTrunk = ArcRevision.of(ArcBranch.trunk().asString());
        var content = arcService.getFileContent(AUTOCHECK_BLACKLIST_PATH, headTrunk);
        if (content.isEmpty()) {
            log.warn("not found {} at {}", AUTOCHECK_BLACKLIST_PATH, headTrunk);
        }
        return parseBlacklist(content.orElse(""));
    }

    private static boolean startsWithAnySubpaths(String path, Set<String> subpaths) {
        return subpaths.stream().anyMatch(subpath -> pathStartsWith(path, subpath));
    }

    static boolean pathStartsWith(@Nonnull String path, @Nonnull String subpath) {
        return path.startsWith(subpath)
                && (path.length() == subpath.length() || path.charAt(subpath.length()) == '/');
    }

    static Set<String> parseBlacklist(String content) {
        return Arrays.stream(content.split("\n"))
                .map(it -> it.replaceFirst("#.*", "").trim())
                .filter(it -> !it.isEmpty())
                .map(AffectedDirectoriesHelper::normalizePath)
                .collect(Collectors.toUnmodifiableSet());
    }

}
