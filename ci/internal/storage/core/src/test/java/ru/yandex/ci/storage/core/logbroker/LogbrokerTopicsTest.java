package ru.yandex.ci.storage.core.logbroker;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.storage.core.CheckOuterClass.CheckType;

import static org.assertj.core.api.Assertions.assertThat;

public class LogbrokerTopicsTest {
    @Test
    public void getsRightTopic() {
        assertThat(LogbrokerTopics.get(ArcBranch.ofString("/branches/a"), ArcBranch.ofString("/branches/a")).getType())
                .isEqualTo(CheckType.BRANCH_POST_COMMIT);

        assertThat(LogbrokerTopics.get(ArcBranch.ofString("trunk"), ArcBranch.ofString("trunk")).getType())
                .isEqualTo(CheckType.TRUNK_POST_COMMIT);

        assertThat(LogbrokerTopics.get(ArcBranch.ofString("/branches/a"), ArcBranch.ofString("pr:42")).getType())
                .isEqualTo(CheckType.BRANCH_PRE_COMMIT);

        assertThat(LogbrokerTopics.get(ArcBranch.ofString("trunk"), ArcBranch.ofString("pr:42")).getType())
                .isEqualTo(CheckType.TRUNK_PRE_COMMIT);
    }
}
