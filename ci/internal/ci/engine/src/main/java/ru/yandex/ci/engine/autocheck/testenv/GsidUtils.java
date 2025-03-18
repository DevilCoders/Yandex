package ru.yandex.ci.engine.autocheck.testenv;

// Class is a copy of https://nda.ya.ru/t/85mEFjdH4Gc4yv
final class GsidUtils {

    private static final String USER = "USER:";
    private static final String ARCANUM = "ARCANUM:";

    private GsidUtils() {
    }

    static String gsidWithAuthorAndReviewRequestId(
            long reviewRequestId, String author, String gsid
    ) {
        var sb = new StringBuilder(gsid);
        if (!gsid.contains(USER)) {
            if (!gsid.isEmpty()) {
                sb.append(' ');
            }
            sb.append(USER).append(author);
        }
        if (!gsid.contains(ARCANUM)) {
            sb.append(' ').append(ARCANUM).append(reviewRequestId);
        }
        return sb.toString();
    }
}
