package ru.yandex.ci.engine.branch;

public class UnknownBranchException extends BranchException {
    public UnknownBranchException(String message) {
        super(message);
    }
}
