package ru.yandex.ci.engine.launch.cleanup;

import org.assertj.core.api.Assertions;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.config.a.model.CleanupReason;
import ru.yandex.commune.bazinga.impl.OnetimeUtils;

class PullRequestDiffSetCompleteTaskTest {

    @Test
    void parseParams() {
        String json = """
                {"pullRequestId":795279,"diffSetId":1586069}
                """;
        var params = OnetimeUtils.parseParameters(new PullRequestDiffSetCompleteTask(0, 0, null), json);
        Assertions.assertThat(params)
                .isEqualTo(new PullRequestDiffSetCompleteTask.Params(795279, 1586069, CleanupReason.FINISH));
    }

}
