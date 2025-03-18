package ru.yandex.ci.engine.branch;

public abstract class BranchException extends RuntimeException {

    BranchException(String message) {
        super(message);
    }
}
