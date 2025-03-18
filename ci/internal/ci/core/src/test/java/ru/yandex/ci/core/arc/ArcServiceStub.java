package ru.yandex.ci.core.arc;

import java.io.IOException;
import java.io.UncheckedIOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ConcurrentHashMap;
import java.util.function.Consumer;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import com.google.common.hash.HashCode;
import com.google.common.hash.Hashing;
import lombok.Value;
import lombok.With;
import one.util.streamex.StreamEx;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.core.io.Resource;
import org.springframework.core.io.support.PathMatchingResourcePatternResolver;

import ru.yandex.arc.api.Repo;
import ru.yandex.arc.api.Repo.ChangelistResponse.ChangeType;
import ru.yandex.arc.api.Shared;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.test.TestUtils;
import ru.yandex.ci.util.ResourceUtils;
import ru.yandex.lang.NonNullApi;

@NonNullApi
public class ArcServiceStub implements ArcService {

    private static final Logger log = LoggerFactory.getLogger(ArcServiceStub.class);

    private String resourcePrefix;
    private List<String> blacklistPrefixes;

    private final Map<String, CommitData> commits = new ConcurrentHashMap<>();

    private final Runnable initializer;

    private Path slice;

    public ArcServiceStub() {
        this.initializer = this::defaultTestData;
        resetAndInitTestData();
    }

    public ArcServiceStub(String resourcePrefix, ArcCommit... commits) {
        this.initializer = () -> {
            this.blacklistPrefixes = List.of();
            this.resourcePrefix = resourcePrefix;
            addCommitAuto(commits);
        };
        resetAndInitTestData();
    }

    public void initRepo(String resourcePrefix, ArcCommit... commits) {
        reset();
        addFirstCommit();
        this.resourcePrefix = resourcePrefix;
        addCommitAuto(commits);
    }

    @Override
    public void close() {
        //
    }

    public void addFirstCommit() {
        //Trunk
        addCommit(TestData.TRUNK_COMMIT_1, null, Map.of());
    }

    @SuppressWarnings("checkstyle:MethodLength")
    private void defaultTestData() {
        this.blacklistPrefixes = List.of("notify-pr-about-security-problems");
        this.resourcePrefix = "test-repo";

        log.info("Init test data");

        var inWithBranchesPath = TestData.CONFIG_PATH_WITH_BRANCHES_RELEASE.getParent().resolve("some.txt");
        var inAuto = TestData.CONFIG_PATH_AUTO_RELEASE_SIMPLE.getParent().resolve("simple.txt");
        var inSawmill = TestData.CONFIG_PATH_SAWMILL_RELEASE.resolve("my.txt");

        //Trunk
        addFirstCommit();

        addCommitAuto(TestData.TRUNK_COMMIT_1);
        addCommitAuto(TestData.TRUNK_COMMIT_2);

        addCommitAuto(
                TestData.TRUNK_COMMIT_3,
                Map.of(
                        inAuto, ChangeType.Modify,
                        inWithBranchesPath, ChangeType.Modify,
                        inSawmill, ChangeType.Modify
                )
        );

        addRelease1BranchCommits();

        addCommitAuto(
                TestData.TRUNK_COMMIT_4,
                Map.of(
                        inWithBranchesPath, ChangeType.Modify,
                        inSawmill, ChangeType.Modify
                )
        );

        addCommitExtended(
                TestData.TRUNK_COMMIT_5,
                Map.of(
                        Path.of("ci/registry/demo/woodflow/sawmill.yaml"), ChangeInfo.of(ChangeType.Modify),
                        TestData.CONFIG_PATH_MOVED_TO, ChangeInfo.of(ChangeType.Move, TestData.CONFIG_PATH_MOVED_FROM),
                        TestData.CONFIG_PATH_PR_NEW, ChangeInfo.of(ChangeType.Add),
                        TestData.PATH_IN_SIMPLE_RELEASE, ChangeInfo.of(ChangeType.Modify),
                        TestData.PATH_IN_SIMPLE_FILTER_RELEASE, ChangeInfo.of(ChangeType.Modify),
                        TestData.CONFIG_PATH_WITH_BRANCHES_RELEASE, ChangeInfo.of(ChangeType.Modify),
                        inSawmill, ChangeInfo.of(ChangeType.Modify),
                        inWithBranchesPath, ChangeInfo.of(ChangeType.Modify)
                )
        );

        addCommit(
                TestData.TRUNK_COMMIT_6,
                Map.of(
                        TestData.PATH_IN_SIMPLE_RELEASE, ChangeType.Modify,
                        TestData.PATH_IN_SIMPLE_FILTER_RELEASE, ChangeType.Modify,
                        TestData.CONFIG_PATH_SAWMILL_RELEASE.resolve("readme.md"), ChangeType.Modify,
                        inWithBranchesPath, ChangeType.Modify
                )
        );

        addRelease2BranchCommits();

        //Pr 42
        addCommit(
                TestData.REVISION_COMMIT,
                TestData.TRUNK_COMMIT_2,
                Map.of(
                        TestData.CONFIG_PATH_PR_NEW, ChangeType.Add,
                        TestData.CONFIG_PATH_ABC, ChangeType.Modify,
                        TestData.CONFIG_PATH_CHANGE_DS1, ChangeType.Modify
                )
        );
        addCommit(
                TestData.SECOND_REVISION_COMMIT,
                TestData.REVISION_COMMIT,
                Map.of(
                        TestData.CONFIG_PATH_PR_NEW, ChangeType.Add
                )
        );
        addCommit(
                TestData.THIRD_REVISION_COMMIT,
                TestData.SECOND_REVISION_COMMIT,
                Map.of(
                        TestData.CONFIG_PATH_PR_NEW, ChangeType.Add,
                        TestData.CONFIG_PATH_ABC, ChangeType.Modify
                )
        );

        // Merge revisions for Pr 42
        addCommitAuto(
                TestData.DS1_COMMIT,
                TestData.TRUNK_COMMIT_2,
                Map.of(
                        TestData.CONFIG_PATH_PR_NEW, ChangeType.Add,
                        TestData.CONFIG_PATH_ABC, ChangeType.Modify,
                        TestData.CONFIG_PATH_CHANGE_DS1, ChangeType.Modify
                )
        );
        addCommit(
                TestData.DS4_COMMIT,
                TestData.TRUNK_COMMIT_2,
                Map.of(
                        TestData.INTERNAL_CHANGE_AYAML_PATH, ChangeType.Modify
                )
        );
        addCommit(
                TestData.DS2_COMMIT,
                TestData.TRUNK_COMMIT_3,
                Map.of(
                        TestData.CONFIG_PATH_PR_NEW, ChangeType.Add,
                        TestData.CONFIG_PATH_ABC, ChangeType.Modify,
                        TestData.CONFIG_PATH_CHANGE_DS1, ChangeType.Modify
                )
        );
        addCommit(
                TestData.DS3_COMMIT,
                TestData.TRUNK_COMMIT_4,
                Map.of(
                        TestData.CONFIG_PATH_PR_NEW, ChangeType.Add,
                        TestData.CONFIG_PATH_ABC, ChangeType.Modify
                )
        );
        addCommit(
                TestData.DS5_COMMIT,
                TestData.TRUNK_COMMIT_2,
                Map.of(
                        Path.of("pr/a.yml"), ChangeType.Modify
                )
        );
        addCommit(
                TestData.DS6_COMMIT,
                TestData.TRUNK_COMMIT_2,
                Map.of(
                        TestData.CONFIG_PATH_PR_NEW, ChangeType.Add
                )
        );
        addCommitAuto(TestData.DS7_COMMIT);

        addCommit(
                TestData.TRUNK_COMMIT_7,
                Map.of(
                        Path.of(
                                "autocheck.fast.targets/first.not.empty/file.dir/a.txt"
                        ), ChangeType.Modify,
                        Path.of(
                                "autocheck.fast.targets/first.not.empty/a.yaml"
                        ), ChangeType.None,
                        TestData.FAST_TARGETS_ROOT_AYAML, ChangeType.None,
                        TestData.PATH_IN_SIMPLE_RELEASE, ChangeType.Modify,
                        TestData.PATH_IN_SIMPLE_FILTER_RELEASE, ChangeType.Modify,
                        inSawmill, ChangeType.Modify,
                        Path.of(
                                "autocheck.native.builds/first.not.empty/file.dir/a.txt"
                        ), ChangeType.Modify,
                        Path.of(
                                "autocheck.native.builds/first.not.empty/a.yaml"
                        ), ChangeType.None,
                        TestData.NATIVE_BUILDS_ROOT_AYAML, ChangeType.None,
                        inWithBranchesPath, ChangeType.Modify
                )
        );

        addCommit(
                TestData.TRUNK_COMMIT_8,
                Map.of(
                        Path.of(
                                "autocheck.fast.targets/first.empty/file.dir/a.txt"
                        ), ChangeType.Modify,
                        Path.of(
                                "autocheck.fast.targets/first.empty/a.yaml"
                        ), ChangeType.None,
                        TestData.FAST_TARGETS_ROOT_AYAML, ChangeType.None,
                        inSawmill, ChangeType.Modify,
                        TestData.AUTOSTART_LARGE_TESTS_SIMPLE_AYAML_PATH, ChangeType.Add,
                        Path.of(
                                "autocheck.native.builds/first.empty/file.dir/a.txt"
                        ), ChangeType.Modify,
                        Path.of(
                                "autocheck.native.builds/first.empty/a.yaml"
                        ), ChangeType.None,
                        TestData.NATIVE_BUILDS_ROOT_AYAML, ChangeType.None,
                        inWithBranchesPath, ChangeType.Modify
                )
        );

        addCommit(
                TestData.TRUNK_COMMIT_9,
                Map.of(
                        Path.of(
                                "autocheck.fast.targets/first.null/file.dir/a.txt"
                        ), ChangeType.Modify,
                        Path.of(
                                "autocheck.fast.targets/first.null/a.yaml"
                        ), ChangeType.None,
                        TestData.FAST_TARGETS_ROOT_AYAML, ChangeType.None,
                        Path.of(
                                "autocheck.fast.targets/sequential/a.yaml"
                        ), ChangeType.None,
                        Path.of(
                                "autocheck.fast.targets/sequential/b.txt"
                        ), ChangeType.Modify,
                        inWithBranchesPath, ChangeType.Modify
                )
        );

        addCommit(
                TestData.TRUNK_COMMIT_10,
                Map.of(
                        Path.of(
                                "autocheck.fast.targets/first.invalid/file.dir/a.txt"
                        ), ChangeType.Modify,
                        TestData.FAST_TARGETS_FIRST_INVALID_FIRST_AYAML_PATH, ChangeType.None,
                        TestData.FAST_TARGETS_ROOT_AYAML, ChangeType.None,
                        TestData.AUTOSTART_LARGE_TESTS_SIMPLE_AYAML_PATH, ChangeType.Add,
                        TestData.AUTOSTART_LARGE_TESTS_FIRST_INVALID_AYAML_PATH, ChangeType.Add,
                        Path.of(
                                "autocheck.native.builds/first.invalid/file.dir/a.txt"
                        ), ChangeType.Modify,
                        TestData.NATIVE_BUILDS_FIRST_INVALID_FIRST_AYAML_PATH, ChangeType.None,
                        TestData.NATIVE_BUILDS_ROOT_AYAML, ChangeType.None
                )
        );

        addCommit(
                TestData.TRUNK_COMMIT_11,
                Map.of(TestData.TRIGGER_ON_COMMIT_AYAML_PATH, ChangeType.Add)
        );

        addCommit(
                TestData.TRUNK_COMMIT_12,
                Map.of(
                        TestData.DISCOVERY_DIR_ON_COMMIT_AYAML_PATH, ChangeType.Add,
                        TestData.DISCOVERY_DIR_ON_COMMIT2_AYAML_PATH, ChangeType.Add,
                        Path.of("discovery-dir-2/value.txt"), ChangeType.Add
                )
        );

        addCommit(
                TestData.TRUNK_COMMIT_13,
                Map.of(
                        Path.of("change/in/dir/and-more/a.txt"), ChangeType.Add
                )
        );

        addCommit(
                TestData.TRUNK_COMMIT_14,
                Map.of(
                        TestData.DISCOVERY_DIR_ON_COMMIT_AYAML_PATH, ChangeType.Modify,
                        Path.of("change/in/dir/and-more/a.txt"), ChangeType.Add
                )
        );

        addCommitAuto(TestData.TRUNK_COMMIT_R2R1);
        addCommitAuto(TestData.TRUNK_COMMIT_R2R2);
    }

    private void addRelease1BranchCommits() {
        var inSawmill = TestData.CONFIG_PATH_SAWMILL_RELEASE.resolve("my.txt");
        addCommit(
                TestData.RELEASE_BRANCH_COMMIT_2_1,
                TestData.TRUNK_COMMIT_2,
                Map.of(
                        TestData.CONFIG_PATH_ABC, ChangeType.Delete,
                        TestData.PATH_IN_SIMPLE_RELEASE, ChangeType.Modify,
                        TestData.PATH_IN_SIMPLE_FILTER_RELEASE, ChangeType.Modify,
                        TestData.CONFIG_PATH_WITH_BRANCHES_RELEASE, ChangeType.Modify,
                        inSawmill, ChangeType.Modify
                )
        );

        addCommit(
                TestData.RELEASE_BRANCH_COMMIT_2_2,
                TestData.RELEASE_BRANCH_COMMIT_2_1,
                Map.of(
                        TestData.PATH_IN_SIMPLE_RELEASE, ChangeType.Modify,
                        TestData.PATH_IN_SIMPLE_FILTER_RELEASE, ChangeType.Modify,
                        inSawmill, ChangeType.Modify
                )
        );
    }

    private void addRelease2BranchCommits() {
        Path fileInSawmill = TestData.CONFIG_PATH_SAWMILL_RELEASE.resolve("readme.md");
        Path fileInWithBranchesPath = TestData.CONFIG_PATH_WITH_BRANCHES_RELEASE.getParent().resolve("some.txt");

        addCommit(
                TestData.RELEASE_BRANCH_COMMIT_6_1,
                TestData.TRUNK_COMMIT_6,
                Map.of(
                        TestData.CONFIG_PATH_ABC, ChangeType.Delete,
                        TestData.PATH_IN_SIMPLE_RELEASE, ChangeType.Modify,
                        TestData.PATH_IN_SIMPLE_FILTER_RELEASE, ChangeType.Modify,
                        fileInSawmill, ChangeType.Modify,
                        fileInWithBranchesPath, ChangeType.Modify
                )
        );

        addCommit(
                TestData.RELEASE_BRANCH_COMMIT_6_2,
                TestData.RELEASE_BRANCH_COMMIT_6_1,
                Map.of(
                        TestData.PATH_IN_SIMPLE_RELEASE, ChangeType.Modify,
                        TestData.PATH_IN_SIMPLE_FILTER_RELEASE, ChangeType.Modify,
                        fileInSawmill, ChangeType.Modify,
                        fileInWithBranchesPath, ChangeType.Modify
                )
        );

        addCommit(
                TestData.RELEASE_BRANCH_COMMIT_6_3,
                TestData.RELEASE_BRANCH_COMMIT_6_2,
                Map.of(
                        TestData.PATH_IN_SIMPLE_RELEASE, ChangeType.Modify,
                        TestData.PATH_IN_SIMPLE_FILTER_RELEASE, ChangeType.Modify,
                        fileInSawmill, ChangeType.Modify,
                        fileInWithBranchesPath, ChangeType.Modify
                )
        );

        addCommit(
                TestData.RELEASE_BRANCH_COMMIT_6_4,
                TestData.RELEASE_BRANCH_COMMIT_6_3,
                Map.of(
                        TestData.PATH_IN_SIMPLE_RELEASE, ChangeType.Modify,
                        TestData.PATH_IN_SIMPLE_FILTER_RELEASE, ChangeType.Modify,
                        fileInSawmill, ChangeType.Modify,
                        fileInWithBranchesPath, ChangeType.Modify
                )
        );
    }

    public void addCommit(ArcCommit commit, @Nullable CommitId previousCommit, Map<Path, ChangeType> changes) {
        addCommitExtended(commit, previousCommit, mapToChangeInfo(changes));
    }

    public void addCommitExtended(ArcCommit commit, @Nullable CommitId previousCommit, Map<Path, ChangeInfo> changes) {
        log.debug(
                "Add {}, previous is {}, content: {}",
                commit, previousCommit, changes
        );
        commits.put(commit.getCommitId(), new CommitData(commit, null, changes, previousCommit));
        if (commit.isTrunk()) {
            makeTrunkPointToRevision(commit);
        }
    }

    public void addCommit(ArcCommit commit, Map<Path, ChangeType> changes) {
        addCommitExtended(commit, mapToChangeInfo(changes));
    }

    public void addCommitExtended(ArcCommit commit, Map<Path, ChangeInfo> changes) {
        addCommitExtended(commit, findPreviousCommit(commit), changes);
    }

    @Nullable
    public CommitId findPreviousCommit(ArcCommit commit) {
        return switch (commit.getParents().size()) {
            case 0 -> null;
            case 1 -> ArcRevision.of(commit.getParents().get(0));
            default -> throw new IllegalArgumentException("expected 1 or 0 parent commits");
        };
    }

    public void addCommitAuto(ArcCommit commit) {
        addCommitAuto(commit, findPreviousCommit(commit), Map.of());
    }

    public void addCommitAuto(ArcCommit... commits) {
        for (var commit : commits) {
            addCommitAuto(commit, findPreviousCommit(commit), Map.of());
        }
    }

    public void addCommitAuto(ArcCommit commit, Map<Path, ChangeType> additionalChanges) {
        addCommitAuto(commit, findPreviousCommit(commit), additionalChanges);
    }

    private void addCommitAuto(ArcCommit commit,
                               @Nullable CommitId previousCommit,
                               Map<Path, ChangeType> additionalChanges) {

        String commitResourcePrefix = getCommitResourcePrefix(commit);
        log.debug(
                "Auto discovering test repository on commit {} in classpath resources {}",
                commit, commitResourcePrefix
        );

        PathMatchingResourcePatternResolver resolver = new PathMatchingResourcePatternResolver();
        Resource[] resources;
        try {
            resources = resolver.getResources(commitResourcePrefix + "**");
        } catch (IOException e) {
            throw new UncheckedIOException(e);
        }
        Map<Path, ChangeType> changes = new HashMap<>(additionalChanges);
        String prefix = ResourceUtils.url(commitResourcePrefix).getFile();
        for (Resource resource : resources) {
            String resourcePath;
            try {
                resourcePath = resource.getURL().getFile();
            } catch (IOException e) {
                throw new UncheckedIOException(e);
            }
            if (resourcePath.endsWith("/")) {
                continue;
            }
            Path pathInRepo = Path.of(resourcePath.replace(prefix, ""));
            if (isBlacklisted(pathInRepo)) {
                log.debug("Skipping repo file {} cause it is blacklisted", pathInRepo);
                continue;
            }
            var changeType = getChangeType(previousCommit, pathInRepo);
            log.debug("File {} was {} on {}", pathInRepo, changeType, commit);
            changes.put(pathInRepo, changeType);
        }
        addCommit(commit, previousCommit, changes);
    }

    private boolean isBlacklisted(Path pathInRepo) {
        for (String blacklistPrefix : blacklistPrefixes) {
            if (pathInRepo.startsWith(blacklistPrefix)) {
                return true;
            }
        }
        return false;
    }

    private ChangeType getChangeType(@Nullable CommitId previousCommit, Path pathInRepo) {
        if (previousCommit == null) {
            return ChangeType.Add;
        }
        if (!commits.containsKey(previousCommit.getCommitId())) {
            return ChangeType.Add;
        }
        if (getFileContent(pathInRepo, previousCommit).isEmpty()) {
            return ChangeType.Add;
        }
        return ChangeType.Modify;
    }

    private String getCommitResourcePrefix(ArcCommit commit) {
        return resourcePrefix + "/" + commit.getCommitId() + "/";
    }

    public void makeTrunkPointToRevision(ArcCommit commit) {
        commits.put(ArcBranch.trunk().asString(), commits.get(commit.getCommitId()));
    }

    public void reset() {
        log.info("Reset test data");
        commits.clear();
        slice = null;
    }

    public void resetAndInitTestData() {
        reset();
        initializer.run();
    }

    /**
     * Задает подмножество состояния тестового репозитория, в котором будет учитываться изменения.
     * Позволяет сузить обнаружение новых затронутых конфигураций до директории с тестируемым конфигом.
     * Use separate test repos instead
     */
    @Deprecated
    public void setSlice(Path slice) {
        this.slice = slice;
    }

    @Deprecated
    public void setSlice(CiProcessId processId) {
        this.slice = processId.getPath().getParent();
    }

    /**
     * Моделирует коммит в транк от какого-то пользователя с каким-то месседжем
     *
     * @param contentPath путь до папки из ресурсов, содержащий файлы, доступные в коммите
     * @param changes     список изменений в коммите
     * @return родительский и созданный коммиты
     */
    @SuppressWarnings("deprecation")
    public CommitResult addTrunkCommit(@Nullable String contentPath, Map<Path, ChangeType> changes) {
        long number = commits.values().stream()
                .map(CommitData::getCommit)
                .filter(ArcCommit::isTrunk)
                .mapToLong(ArcCommit::getSvnRevision)
                .max()
                .orElse(1) + 1;
        String message = "test dummy commit #" + number;
        HashCode hashCode = Hashing.sha1().hashString(message, StandardCharsets.UTF_8);
        OrderedArcRevision trunk = OrderedArcRevision.fromHash(hashCode.toString(), "trunk", number, 0);

        ArcCommit current = TestData.toCommit(trunk, TestData.CI_USER);
        addTrunkCommit(current, contentPath, changes);
        return new CommitResult(current);
    }

    /**
     * Добавляет коммит в trunk
     *
     * @return коммит, поверх которого был сделан текущий (родительский относительно сделанного)
     */
    public Optional<ArcCommit> addTrunkCommit(
            ArcCommit commit,
            @Nullable String contentPath,
            Map<Path, ChangeType> changes
    ) {
        return addTrunkCommitExtended(commit, contentPath, mapToChangeInfo(changes));
    }

    public Optional<ArcCommit> addTrunkCommitExtended(
            ArcCommit commit,
            @Nullable String contentPath,
            Map<Path, ChangeInfo> changes
    ) {
        Optional<CommitData> latest = commits.values().stream()
                .filter(c -> c.getCommit().isTrunk())
                .max(Comparator.comparing(c -> c.getCommit().getSvnRevision()));

        Preconditions.checkArgument(
                latest.isEmpty() || commit.getSvnRevision() >= latest.get().getCommit().getSvnRevision(),
                "there are commit has same or greater number %s >= %s",
                latest,
                commit
        );

        String previousCommit = latest.map(c -> c.getCommit().getCommitId()).orElse(null);
        commits.put(commit.getCommitId(), new CommitData(commit, null, changes, previousCommit, contentPath));
        return latest.map(CommitData::getCommit);
    }

    @Override
    public List<ArcCommitWithPath> getCommits(
            CommitId laterStartRevision,
            @Nullable CommitId earlierStopRevision,
            @Nullable Path path,
            int limit
    ) {
        List<CommitData> commitsInternal = getCommitsInternal(earlierStopRevision, laterStartRevision, path, limit);
        return commitsInternal.stream()
                .map(data -> new ArcCommitWithPath(data.commit, data.path))
                .collect(Collectors.toList());
    }

    @Override
    public List<ArcCommit> getBranchCommits(CommitId startRevision, @Nullable Path path) {
        List<ArcCommit> result = new ArrayList<>();
        CommitData commit = commits.get(startRevision.getCommitId());
        do {
            if (acceptCommit(commit, path)) {
                result.add(commit.getCommit());
            }
            if (commit.getCommit().isTrunk()) {
                break;
            }
            commit = getCommitNullable(commit.getPreviousCommit());
        } while (commit != null);
        return result;
    }

    private boolean acceptCommit(CommitData commitData, @Nullable Path pathFilter) {
        if (pathFilter == null) {
            return true;
        }
        log.debug("Path [{}] matching against {} ({})", pathFilter, commitData.changes.keySet(), commitData.commit);
        for (var path : commitData.changes.keySet()) {
            if (path.startsWith(pathFilter)) {
                log.debug("Matched with [{}]", path);
                return true;
            }
        }
        log.debug("Not matched");
        return false;
    }

    private List<CommitData> getCommitsInternal(
            @Nullable CommitId earlierCommit,
            CommitId laterCommit,
            @Nullable Path path,
            int limit
    ) {
        if (limit <= 0) {
            limit = Integer.MAX_VALUE;
        }

        String stopCommitId = earlierCommit == null ? null : earlierCommit.getCommitId();

        List<CommitData> result = new ArrayList<>();
        CommitData commit = commits.get(laterCommit.getCommitId());
        while (result.size() < limit && commit != null && !commit.getCommit().getCommitId().equals(stopCommitId)) {
            if (path == null) {
                result.add(commit);
            } else if (acceptCommit(commit, path)) {
                result.add(commit
                        .withCommit(commit.getCommit())
                        .withPath(path));
            }
            commit = getCommitNullable(commit.getPreviousCommit());
        }

        return result;
    }

    @Override
    public ArcCommit getCommit(CommitId revision) {
        CommitData commitData = commits.get(revision.getCommitId());
        if (commitData == null) {
            throw CommitNotFoundException.fromCommitId(revision);
        }
        return commitData.getCommit();
    }

    @Override
    public boolean isCommitExists(CommitId revision) {
        try {
            getCommit(revision);
        } catch (CommitNotFoundException e) {
            return false;
        }
        return true;
    }

    @Override
    public ArcRevision getLastRevisionInBranch(ArcBranch branch) {
        if (!branch.isTrunk()) {
            throw new UnsupportedOperationException();
        }
        return StreamEx.ofValues(commits)
                .map(CommitData::getCommit)
                .filter(ArcCommit::isTrunk)
                .maxByLong(ArcCommit::getSvnRevision)
                .orElseThrow()
                .getRevision();
    }

    @Override
    public void processChanges(CommitId laterFromCommit,
                               @Nullable CommitId earlierToCommit,
                               @Nullable Path pathFilterDir,
                               Consumer<Repo.ChangelistResponse.Change> changeConsumer) {
        getCommitsInternal(earlierToCommit, laterFromCommit, null, earlierToCommit != null ? -1 : 1)
                .stream()
                .filter(c -> acceptCommit(c, pathFilterDir))
                .flatMap(c -> c.getChanges().entrySet().stream())
                .filter(e -> e.getValue().getType() != ChangeType.None)
                .filter(e -> slice == null || e.getKey().startsWith(slice))
                .filter(e -> pathFilterDir == null || e.getKey().startsWith(pathFilterDir))
                .map(e -> toChange(e.getKey(), e.getValue()))
                .forEach(changeConsumer);
    }

    private static Repo.ChangelistResponse.Change toChange(Path path, ChangeInfo info) {
        var builder = Repo.ChangelistResponse.Change.newBuilder()
                .setChange(info.getType())
                .setPath(path.toString());
        if (info.getSourcePath() != null) {
            builder.setSource(Repo.ChangelistResponse.Source.newBuilder()
                    .setPath(info.getSourcePath().toString())
                    .build()
            );
        }
        return builder.build();
    }


    @Override
    public Optional<String> getFileContent(String path, CommitId revision) {
        Path filePath = Path.of(path);
        CommitData commit = commits.get(revision.getCommitId());
        Preconditions.checkState(commit != null, "No revision %s", revision);
        while (commit != null) {
            if (commit.getChanges().containsKey(filePath)) {
                log.debug("Found file {} at revision {}", path, revision);

                //TODO обработать свякие разные change type, типа Delete
                if (commit.getCustomContentPath() != null) {
                    return Optional.of(
                            TestUtils.textResource(commit.getCustomContentPath() + "/" + filePath)
                    );
                }
                return Optional.of(
                        TestUtils.textResource(getCommitResourcePrefix(commit.getCommit()) + filePath)
                );
            }
            commit = getCommitNullable(commit.getPreviousCommit());
        }

        log.debug("Not found file {} at revision {}. resourcePrefix: {}", path, revision, resourcePrefix);
        return Optional.empty();
    }

    @Override
    public Optional<RepoStat> getStat(String path, CommitId revision, boolean lastChanged) {
        var oid = getLastCommit(Path.of(path), revision)
                .map(it -> it.getRevision().getCommitId())
                .orElse("");
        return getFileContent(path, revision)
                .map(it -> new RepoStat(
                        Path.of(path).getFileName().toString(),
                        RepoStat.EntryType.FILE,
                        0, false, false, oid, null, false
                ));
    }

    @Override
    public CompletableFuture<Optional<RepoStat>> getStatAsync(String path, CommitId revision, boolean lastChanged) {
        return CompletableFuture.completedFuture(
                getStat(path, revision, lastChanged)
        );
    }

    @Override
    public List<Path> listDir(String path, CommitId commitId, boolean recursive, boolean onlyFiles) {
        return List.of(Path.of("test"));
    }

    @Override
    public void resetBranch(ArcBranch branch, CommitId commitId, String oauthToken) {
        throw new UnsupportedOperationException();
    }

    @Override
    public Shared.Commit commitFile(String path, ArcBranch branch, CommitId expectedHead, String message,
                                    String content, String token) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void createReleaseBranch(CommitId commitId, ArcBranch branch, String oauthToken) {
        log.debug("called method to created branch {} at {}", branch, commitId);
    }

    @Override
    public List<String> getBranches(String branchPrefix) {
        return Stream.of(
                        TestData.RELEASE_BRANCH_1.asString(),
                        TestData.RELEASE_BRANCH_2.asString(),
                        ArcBranch.trunk().asString()
                ).filter(it -> it.startsWith(branchPrefix))
                .collect(Collectors.toList());
    }

    @Nullable
    private CommitData getCommitNullable(@Nullable String commit) {
        return commit != null
                ? commits.get(commit)
                : null;
    }

    private static Map<Path, ChangeInfo> mapToChangeInfo(Map<Path, ChangeType> changes) {
        return changes.entrySet().stream().collect(Collectors.toMap(
                Map.Entry::getKey,
                it -> ChangeInfo.of(it.getValue())
        ));
    }

    @Value
    @SuppressWarnings("ReferenceEquality")
    private static class CommitData {
        @With
        ArcCommit commit;

        @With
        @Nullable
        Path path;

        Map<Path, ChangeInfo> changes;

        @Nullable
        String previousCommit;

        @Nullable
        String customContentPath;

        private CommitData(ArcCommit commit,
                           @Nullable Path path,
                           Map<Path, ChangeInfo> changes,
                           @Nullable CommitId previousCommit) {
            this(commit, path, changes, previousCommit == null ? null : previousCommit.getCommitId(), null);
        }

        private CommitData(ArcCommit commit,
                           @Nullable Path path,
                           Map<Path, ChangeInfo> changes,
                           @Nullable String previousCommit,
                           @Nullable String customContentPath) {
            this.changes = changes;
            if (previousCommit == null) {
                this.commit = commit;
                this.path = path;
                this.previousCommit = null;
            } else {
                this.commit = commit.withParents(List.of(previousCommit));
                this.path = path;
                this.previousCommit = previousCommit;
            }
            this.customContentPath = customContentPath;
        }
    }

    @Value
    public static class CommitResult {
        ArcCommit created;
    }

    @Override
    public Optional<ArcRevision> getMergeBase(CommitId a, CommitId b) {
        Set<CommitData> seen = new HashSet<>();
        CommitData dataA = commits.get(a.getCommitId());
        CommitData dataB = commits.get(b.getCommitId());

        while (dataA != null) {
            seen.add(dataA);
            dataA = getCommitNullable(dataA.getPreviousCommit());
        }

        while (dataB != null) {
            if (seen.contains(dataB)) {
                return Optional.of(dataB.getCommit().getRevision());
            }
            dataB = getCommitNullable(dataB.getPreviousCommit());
        }
        return Optional.empty();
    }

    @Value(staticConstructor = "of")
    public static class ChangeInfo {
        ChangeType type;
        @Nullable
        Path sourcePath;

        public static ChangeInfo of(ChangeType type) {
            return ChangeInfo.of(type, null);
        }
    }

}
