package ru.yandex.ci.observer.core.db.model.check_iterations;

import lombok.Value;

import ru.yandex.ci.storage.core.CheckIteration;

@Value
public class LinkName {
    public static final String CI_BADGE = "CI_BADGE";
    public static final String SANDBOX = "SANDBOX";
    public static final String DISTBUILD = "DISTBUILD";
    public static final String DISTBUILD_VIEWER = "DISTBUILD_VIEWER";
    public static final String REVIEW = "REVIEW";
    public static final String FLOW = "FLOW";

    private LinkName() {
    }

    public static String createFlowLinkName(CheckIteration.IterationId iterationId) {
        var iterationNumber = iterationId.getNumber();
        return createFlowLinkName(iterationNumber);
    }

    private static String createFlowLinkName(int iterationNumber) {
        return iterationNumber == 1 ? FLOW : FLOW + " " + iterationNumber;
    }

}
