package yandex.cloud.dashboard.model.spec.dashboard;

import com.google.common.base.Preconditions;

import static yandex.cloud.util.Predicates.oneOf;

/**
 * @author ssytnik
 */
public class Uids {

    public static String validate(String uid) {
        Preconditions.checkArgument(uid != null && uid.length() <= 40,
                "Uid should be set and have length <= 40, but got '%s'", uid);
        return uid;
    }

    public static String resolve(String rootUid, String subUid) {
        Preconditions.checkArgument(rootUid != null && subUid != null,
                "Both rootUid and subUid should be specified, but got '%s', '%s'", rootUid, subUid);
        return validate(rootUid + "_" + subUid);
    }

    public static String resolve(String uid, String rootUid, String subUid) {
        Preconditions.checkArgument(oneOf(uid, subUid),
                "Either uid or subUid should be specified, but not both: '%s', '%s'", uid, subUid);
        if (uid != null) {
            return validate(uid);
        } else {
            return resolve(rootUid, subUid);
        }
    }

}
