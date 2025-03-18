package ru.yandex.ci.storage.api.util;

import java.util.HashMap;
import java.util.Map;
import java.util.Optional;
import java.util.function.Supplier;

import lombok.AllArgsConstructor;
import org.apache.commons.text.StringSubstitutor;

import ru.yandex.ci.core.exceptions.CiNotFoundException;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.constant.ResultTypeUtils;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffEntity;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;

@AllArgsConstructor
public class TestRunCommandProvider {
    private static final String OPEN_BRACKET = "{{";
    private static final String CLOSE_BRACKET = "}}";

    // Templates from CI-2743
    private static final String BUILD_TEMPLATE = "ya make -DAUTOCHECK=yes {{path}}";

    private static final String STYLE_SUITE_TEMPLATE = "ya make -DAUTOCHECK=yes --style {{path}}";

    private static final String STYLE_TEMPLATE =
            "ya make -DAUTOCHECK=yes --style -F '{{name}}::{{subtest_name}}' {{path}}";

    private static final String SUITE_TEMPLATE =
            "ya make -DAUTOCHECK=yes -A --ignore-recurses --test-size {{size}} --test-type " +
                    "{{suite name}} {{path}}";

    private static final String CHUNK_TEMPLATE =
            "ya make -DAUTOCHECK=yes -A --ignore-recurses --test-size {{size}} --test-type " +
                    "{{suite name}} -F '{{chunk subtest_name}}' {{path}}";

    private static final String TEST_TEMPLATE =
            "ya make -DAUTOCHECK=yes -A --ignore-recurses --test-size {{size}} --test-type " +
                    "{{suite name}} -F '{{chunk subtest_name}}' -F '{{name}}::{{subtest_name}}' {{path}}";

    private static final String TEST_WITHOUT_CHUNK_TEMPLATE =
            "ya make -DAUTOCHECK=yes -A --ignore-recurses --test-size {{size}} " +
                    "--test-type {{suite name}} -F '{{name}}::{{subtest_name}}' {{path}}";

    private final CiStorageDb db;

    public String getRunCommand(TestDiffEntity.Id diffId) {
        return this.db.currentOrReadOnly(() -> {
            var diff = db.testDiffs().get(diffId);

            return getRunCommandInTx(
                    diffId.getResultType(),
                    diff.isChunk(),
                    diffId.getPath(),
                    diff.getName(),
                    diff.getSubtestName(),
                    () -> db.testDiffs().get(diffId.toSuiteId()).getName(),
                    () -> {
                        if (diff.getAutocheckChunkId() == 0) {
                            return Optional.empty();
                        } else {
                            return Optional.of(db.testDiffs().get(diff.getAutocheckChunkDiffId()).getSubtestName());
                        }
                    }
            );
        });
    }

    public String getRunCommand(TestStatusEntity.Id statusId) {
        return this.db.currentOrReadOnly(() -> {
            var test = db.tests().find(statusId.getTestId(), statusId.getSuiteId(), statusId.getBranch())
                    .stream().findFirst().orElseThrow(CiNotFoundException::new);

            return getRunCommandInTx(
                    test.getType(),
                    test.isChunk(),
                    test.getPath(),
                    test.getName(),
                    test.getSubtestName(),
                    () -> db.tests().get(test.getId().toSuiteId()).getName(),
                    () -> {
                        if (test.getAutocheckChunkId() == 0) {
                            return Optional.empty();
                        } else {
                            return Optional.of(
                                    db.tests().get(test.getId().toChunkId(test.getAutocheckChunkId())).getSubtestName()
                            );
                        }
                    }
            );
        });
    }

    public String getRunCommandInTx(
            Common.ResultType resultType,
            boolean isChunk,
            String path,
            String name,
            String subtestName,
            Supplier<String> suiteNameLoader,
            Supplier<Optional<String>> chunkSubtestNameLoader
    ) {
        if (resultType.equals(Common.ResultType.RT_TEST_TESTENV)) {
            return "";
        }

        var values = new HashMap<String, String>();
        values.put("path", path);

        if (ResultTypeUtils.isBuild(resultType) || ResultTypeUtils.isConfigure(resultType)) {
            return replace(BUILD_TEMPLATE, values);
        }

        if (ResultTypeUtils.isStyleSuite(resultType)) {
            return replace(STYLE_SUITE_TEMPLATE, values);
        }

        values.put("name", name);
        values.put("subtest_name", subtestName);

        if (ResultTypeUtils.isStyle(resultType)) {
            return replace(STYLE_TEMPLATE, values);
        }

        values.put("size", ResultTypeUtils.toTestSize(resultType));
        if (ResultTypeUtils.isSuite(resultType)) {
            values.put("suite name", name);
            return replace(SUITE_TEMPLATE, values);
        }

        var suiteName = suiteNameLoader.get();
        values.put("suite name", suiteName);

        if (isChunk) {
            values.put("chunk subtest_name", subtestName);
            return replace(CHUNK_TEMPLATE, values);
        } else {
            var chunkName = chunkSubtestNameLoader.get();
            if (chunkName.isEmpty() ||
                    // todo remove after https://st.yandex-team.ru/DEVTOOLS-9283
                    suiteName.equals("pytest") || suiteName.equals("py3test")
            ) {
                return replace(TEST_WITHOUT_CHUNK_TEMPLATE, values);
            } else {
                values.put("chunk subtest_name", chunkName.get());
                return replace(TEST_TEMPLATE, values);
            }
        }
    }

    private static String replace(String template, Map<String, String> values) {
        return StringSubstitutor.replace(template, values, OPEN_BRACKET, CLOSE_BRACKET);
    }
}
