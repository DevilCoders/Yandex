package ru.yandex.ci.engine.branch;

public class BranchConflictException extends BranchException {
    public BranchConflictException(String message) {
        super(message);
    }
}
