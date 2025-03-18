package ru.yandex.ci.core.arc;

public class CommitNotFoundException extends RuntimeException {
    public CommitNotFoundException(String message) {
        super(message);
    }

    public static CommitNotFoundException fromCommitId(CommitId revision) {
        return new CommitNotFoundException("Commit not found %s".formatted(revision));
    }

}
