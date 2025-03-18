package ru.yandex.ci.engine.launch.auto;

import java.util.Comparator;

public class ResultComparator implements Comparator<Result> {
    private static final ResultComparator INSTANCE = new ResultComparator();

    private static final Comparator<Result> DELEGATE = Comparator.comparing(Result::getAction)
            .thenComparing((o1, o2) -> {
                if (o1.getAction() == Action.SCHEDULE) {
                    return o1.getScheduledAt().compareTo(o2.getScheduledAt());
                }
                return 0;
            });

    private ResultComparator() {
    }

    public static ResultComparator instance() {
        return INSTANCE;
    }

    @Override
    public int compare(Result o1, Result o2) {
        return DELEGATE.compare(o1, o2);
    }
}
