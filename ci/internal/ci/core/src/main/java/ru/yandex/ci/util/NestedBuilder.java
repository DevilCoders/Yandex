package ru.yandex.ci.util;

import java.util.function.Function;

public abstract class NestedBuilder<Parent, TargetObject> {

    private final Function<TargetObject, Parent> toParent;

    protected NestedBuilder(Function<TargetObject, Parent> toParent) {
        this.toParent = toParent;
    }

    protected abstract TargetObject build();

    public Parent end() {
        return toParent.apply(build());
    }
}
