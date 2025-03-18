package ru.yandex.ci.engine.launch;

import javax.annotation.Nullable;

import lombok.Value;

@Value
public class CanStartLaunchAtRevision {
    boolean allowed;
    @Nullable
    String reason;

    public static CanStartLaunchAtRevision allowed() {
        return new CanStartLaunchAtRevision(true, null);
    }

    public static CanStartLaunchAtRevision forbidden(String reason) {
        return new CanStartLaunchAtRevision(false, reason);
    }

}
