package ru.yandex.ci.core.arc;

import java.util.List;

import org.junit.jupiter.api.Test;
import org.mockito.Mockito;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doReturn;

class ArcBranchCacheTest {

    @Test
    void getBranchesBySubstring() {
        var branches = List.of("trunk", "users/albazh/test-branch", "users/42/test");

        assertThat(ArcBranchCache.getBranchesBySubstring(branches, ""))
                .containsExactly("trunk", "users/albazh/test-branch", "users/42/test");

        assertThat(ArcBranchCache.getBranchesBySubstring(branches, "u"))
                .containsExactly("trunk", "users/albazh/test-branch", "users/42/test");
        assertThat(ArcBranchCache.getBranchesBySubstring(branches, "test"))
                .containsExactly("users/albazh/test-branch", "users/42/test");

        assertThat(ArcBranchCache.getBranchesBySubstring(List.of("a", "b"), ""))
                .containsExactly("a", "b");
    }

    @Test
    void getBranchByName() {
        var branches = List.of("trunk", "users/albazh/test-branch", "users/42/test");
        assertThat(ArcBranchCache.getBranchByName(branches, "")).isEmpty();

        assertThat(ArcBranchCache.getBranchByName(branches, "u")).isEmpty();
        assertThat(ArcBranchCache.getBranchByName(branches, "users/42/test"))
                .containsExactly("users/42/test");

        assertThat(ArcBranchCache.getBranchByName(List.of("a", "b"), "a"))
                .containsExactly("a");
    }

    @Test
    void getBranchesByPrefix() {
        var branches = List.of("trunk", "users/albazh/test-branch", "users/42/test");
        assertThat(ArcBranchCache.getBranchesByPrefix(branches, ""))
                .containsExactly("trunk", "users/albazh/test-branch", "users/42/test");

        assertThat(ArcBranchCache.getBranchesByPrefix(branches, "u"))
                .containsExactly("users/albazh/test-branch", "users/42/test");
        assertThat(ArcBranchCache.getBranchesByPrefix(branches, "test")).isEmpty();

        assertThat(ArcBranchCache.getBranchesByPrefix(List.of("a", "b"), ""))
                .containsExactly("a", "b");
    }

    @Test
    void suggestBranches() {
        var arcService = Mockito.mock(ArcService.class);
        doReturn(
                List.of("migrations/123/trunk/123", "trunk", "trunk/123", "trunz/123", "users/42/test")
        ).when(arcService).getBranches(eq(""));

        var arcBranchCache = new ArcBranchCache(arcService);
        arcBranchCache.updateCache();
        assertThat(arcBranchCache.suggestBranches("trunk").limit(20).toList())
                .isEqualTo(List.of("trunk", "trunk/123", "migrations/123/trunk/123"));

        assertThat(arcBranchCache.suggestBranches("trunk").limit(1).toList())
                .isEqualTo(List.of("trunk"));

        assertThat(arcBranchCache.suggestBranches("trunk").limit(2).toList())
                .isEqualTo(List.of("trunk", "trunk/123"));

        assertThat(arcBranchCache.suggestBranches("trunk").limit(3).toList())
                .isEqualTo(List.of("trunk", "trunk/123", "migrations/123/trunk/123"));

        assertThat(arcBranchCache.suggestBranches("trunk").skip(1).limit(1).toList())
                .isEqualTo(List.of("trunk/123"));

        assertThat(arcBranchCache.suggestBranches("trunk").skip(1).limit(2).toList())
                .isEqualTo(List.of("trunk/123", "migrations/123/trunk/123"));

        assertThat(arcBranchCache.suggestBranches("trunk").skip(1).limit(3).toList())
                .isEqualTo(List.of("trunk/123", "migrations/123/trunk/123"));

        assertThat(arcBranchCache.suggestBranches("trunk").skip(2).limit(2).toList())
                .isEqualTo(List.of("migrations/123/trunk/123"));

        assertThat(arcBranchCache.suggestBranches("trunk").skip(3).limit(2).toList())
                .isEmpty();
    }

}
