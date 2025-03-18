package ru.yandex.ci.core.autocheck;

import java.nio.file.Path;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.NavigableSet;
import java.util.TreeSet;

import com.google.common.base.Preconditions;

import ru.yandex.ci.core.config.CiProcessId;

public final class AutocheckConstants {

    public static final Path AUTOCHECK_A_YAML_PATH = Path.of("autocheck/a.yaml");

    public static final CiProcessId STRESS_TEST_TRUNK_PRECOMMIT_PROCESS_ID = CiProcessId.ofFlow(
            AUTOCHECK_A_YAML_PATH, "stress-test-autocheck-trunk-precommits"
    );

    public static final CiProcessId STRESS_TEST_PRECOMMIT_TRUNK_RECHECK_PROCESS_ID = CiProcessId.ofFlow(
            AUTOCHECK_A_YAML_PATH, "stress-test-autocheck-trunk-precommits-recheck"
    );

    public static final String AUTOCHECK_BUILD_PARENT_2 = "AUTOCHECK_BUILD_PARENT_2";

    public static final NavigableSet<String> FULL_CIRCUIT_TARGETS = Collections
            .unmodifiableNavigableSet(new TreeSet<>(List.of("autocheck")));

    private AutocheckConstants() {
    }

    public static class PostCommits {

        public static final String DEFAULT_POOL = "autocheck/postcommits/public";
        public static final String NAMESPACE = "ci.autocheck.postcommits";
        public static final String ENABLED = "enabled";

        private static final Map<String, String> CONFIGURATION_ID_TO_TESTENV_TEST_NAME;

        static {
            /* Order is important (at least while testenv creates post commit checks),
            cause it affects `main_job_partitions` sandbox parameter */
            var map = new LinkedHashMap<String, String>();
            map.put("linux", "BUILD_TRUNK_META_LINUX_DISTBUILD");
            map.put("mandatory", "AUTOCHECK_MANDATORY_PLATFORMS");
            map.put("ios-android-cygwin", "AUTOCHECK_FOR_IOS_ANDROID_CYGWIN");
            map.put("gcc-msvc-musl", "AUTOCHECK_FOR_GCC_MSVC_MUSL");
            map.put("sanitizers", "AUTOCHECK_FOR_SANITIZERS");
            map.put("aux-subset-3", "AUTOCHECK_AUX_SUBSET_3");
            CONFIGURATION_ID_TO_TESTENV_TEST_NAME = Collections.unmodifiableMap(map);
        }

        public static String getTestenvTestNameByConfigurationId(String configurationId) {
            var testName = CONFIGURATION_ID_TO_TESTENV_TEST_NAME.get(configurationId);
            Preconditions.checkState(testName != null, "Can't find testenvTestName for %s", configurationId);
            return testName;
        }

    }

}
