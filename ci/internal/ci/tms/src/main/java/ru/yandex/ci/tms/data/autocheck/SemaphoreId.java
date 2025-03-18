package ru.yandex.ci.tms.data.autocheck;

public enum SemaphoreId {
    AUTOCHECK("12622386", "sem_autocheck_capacity"),
    AUTOCHECK_FOR_BRANCH("16101546", "sem_autocheck_for_branch_capacity"),
    PRECOMMITS("12621000", "sem_precommits_capacity");

    private final String sandboxId;
    private final String stringId;

    SemaphoreId(String sandboxId, String stringId) {
        this.sandboxId = sandboxId;
        this.stringId = stringId;
    }

    public String getSandboxId() {
        return sandboxId;
    }

    public String getStringId() {
        return stringId;
    }
}
