package ru.yandex.ci.engine.launch.version;

import ru.yandex.ci.core.launch.versioning.Slot;

enum SlotFor {
    BRANCH,
    RELEASE;

    public boolean matches(Slot slot) {
        return switch (this) {
            case BRANCH -> !slot.isInBranch();
            case RELEASE -> !slot.isInRelease();
        };
    }

    public Slot occupy(Slot slot) {
        return switch (this) {
            case BRANCH -> slot.withInBranch(true);
            case RELEASE -> slot.withInRelease(true);
        };
    }
}
