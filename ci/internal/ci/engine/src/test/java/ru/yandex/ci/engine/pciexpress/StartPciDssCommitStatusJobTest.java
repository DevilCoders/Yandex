package ru.yandex.ci.engine.pciexpress;

import org.assertj.core.api.Assertions;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.CommonTestBase;

class StartPciDssCommitStatusJobTest extends CommonTestBase {

    private static final String FILE_CONTENT = """
              Projects {
              Name: "Test data"
              AbcService: "Test"
              Packages: "some/path/to/package.json"
              Packages: "some/path/to/next_package.json"
              Packages: "another/path/to/package.json"
              Packages: "another/path/to/another_package.json"
              BranchPrefixes: "users/login/test-"
              BranchPrefixes: "releases/experimental/project/TEST-"
            }
            Approvers: "user007"
            Approvers: "agent007"
            Approvers: "bruce_willis"
            """;

    @Test
    void getTaskConfiguration() throws Exception {
        var configuration = StartPciDssCommitStatusJob.getTaskConfiguration(FILE_CONTENT, "some_commit_hash");
        Assertions.assertThat(configuration)
                .isEqualTo(
                        """
                                CommitId: "some_commit_hash"
                                Packages {
                                  Package: "some/path/to/package.json"
                                }
                                Packages {
                                  Package: "some/path/to/next_package.json"
                                }
                                Packages {
                                  Package: "another/path/to/package.json"
                                }
                                Packages {
                                  Package: "another/path/to/another_package.json"
                                }
                                """
                );
    }
}
