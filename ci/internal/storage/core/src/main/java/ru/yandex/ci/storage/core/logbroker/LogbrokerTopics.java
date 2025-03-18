package ru.yandex.ci.storage.core.logbroker;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.storage.core.CheckOuterClass.CheckType;

public final class LogbrokerTopics {
    private static final LogbrokerTopic BRANCH_PRE_COMMIT = new LogbrokerTopic(
            CheckType.BRANCH_PRE_COMMIT, "/ci/autocheck/stable/main/branch/precommit", 10
    );

    private static final LogbrokerTopic BRANCH_POST_COMMIT = new LogbrokerTopic(
            CheckType.BRANCH_POST_COMMIT, "/ci/autocheck/stable/main/branch/postcommit", 10
    );

    private static final LogbrokerTopic TRUNK_PRE_COMMIT = new LogbrokerTopic(
            CheckType.TRUNK_PRE_COMMIT, "/ci/autocheck/stable/main/trunk/precommit", 150
    );

    private static final LogbrokerTopic TRUNK_POST_COMMIT = new LogbrokerTopic(
            CheckType.TRUNK_POST_COMMIT, "/ci/autocheck/stable/main/trunk/postcommit", 60
    );

    private LogbrokerTopics() {

    }

    public static LogbrokerTopic get(ArcBranch left, ArcBranch right) {
        if (right.isTrunk()) {
            return LogbrokerTopics.TRUNK_POST_COMMIT;
        }

        if (right.isPr()) {
            return left.isTrunk() ? LogbrokerTopics.TRUNK_PRE_COMMIT : LogbrokerTopics.BRANCH_PRE_COMMIT;
        }

        return LogbrokerTopics.BRANCH_POST_COMMIT;
    }

    public static LogbrokerTopic get(CheckType type) {
        return switch (type) {
            case BRANCH_PRE_COMMIT -> LogbrokerTopics.BRANCH_PRE_COMMIT;
            case BRANCH_POST_COMMIT -> LogbrokerTopics.BRANCH_POST_COMMIT;
            case TRUNK_PRE_COMMIT -> LogbrokerTopics.TRUNK_PRE_COMMIT;
            case TRUNK_POST_COMMIT -> LogbrokerTopics.TRUNK_POST_COMMIT;
            case UNRECOGNIZED -> throw new RuntimeException();
        };
    }
}
