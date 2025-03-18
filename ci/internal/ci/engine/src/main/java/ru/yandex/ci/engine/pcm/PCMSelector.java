package ru.yandex.ci.engine.pcm;

import java.nio.file.Path;
import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nullable;

import com.google.common.annotations.VisibleForTesting;

import ru.yandex.ci.core.abc.AbcService;
import ru.yandex.ci.core.db.autocheck.model.AccessControl;

public class PCMSelector {
    @VisibleForTesting
    public static final String PRECOMMITS_DEFAULT_POOL = "autocheck/precommits/public";
    static final String PRECOMMITS_COMMON_COMP_PATH_POOL = "autocheck/precommits/common_components";
    private static final List<Path> COMMON_COMP_PATHS = List.of(
            Path.of("build"),
            Path.of("contrib"),
            Path.of("library"),
            Path.of("util"),
            Path.of("tools/enum_parser")
    );

    private final AbcService abcService;
    private final PCMService pcmService;

    public PCMSelector(AbcService abcService, PCMService pcmService) {
        this.abcService = abcService;
        this.pcmService = pcmService;
    }

    public String selectPool(Set<Path> affectedPaths, @Nullable String authorLogin) {
        if (authorLogin == null) {
            return PRECOMMITS_DEFAULT_POOL;
        }
        if (hasCommonComponents(affectedPaths)) {
            return PRECOMMITS_COMMON_COMP_PATH_POOL;
        }

        return getAvailablePoolNodes(authorLogin).stream()
                .filter(pn -> !pn.getPoolPath().equals(PRECOMMITS_DEFAULT_POOL))
                .filter(pn -> !pn.getPoolPath().equals(PRECOMMITS_COMMON_COMP_PATH_POOL))
                .max(PoolNode::compare)
                .map(PoolNode::getPoolPath)
                .orElse(PRECOMMITS_DEFAULT_POOL);
    }

    private boolean hasCommonComponents(Set<Path> affectedPaths) {
        return affectedPaths.stream()
                .anyMatch(path -> COMMON_COMP_PATHS.stream().anyMatch(path::startsWith));
    }

    private List<PoolNode> getAvailablePoolNodes(String authorLogin) {
        List<AccessControl> acl = Stream.concat(
                abcService.getServicesMembers(authorLogin).stream()
                        .flatMap(member -> Stream.of(
                                AccessControl.ofAbc(member.getServiceSlug(), member.getRoleCode()),
                                AccessControl.ofAbc(member.getServiceSlug(), "")
                        ))
                        .distinct(),
                Stream.of(AccessControl.ofUser(authorLogin), AccessControl.ALL_STAFF)
        ).collect(Collectors.toList());

        return pcmService.getAvailablePools(acl);
    }
}
