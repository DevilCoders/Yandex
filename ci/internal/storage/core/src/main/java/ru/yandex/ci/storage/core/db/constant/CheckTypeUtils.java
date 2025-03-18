package ru.yandex.ci.storage.core.db.constant;

import ru.yandex.ci.storage.core.CheckOuterClass;

public class CheckTypeUtils {
    private CheckTypeUtils() {

    }

    public static boolean isPrecommitCheck(CheckOuterClass.CheckType checkType) {
        return checkType == CheckOuterClass.CheckType.TRUNK_PRE_COMMIT
                || checkType == CheckOuterClass.CheckType.BRANCH_PRE_COMMIT;
    }
}
