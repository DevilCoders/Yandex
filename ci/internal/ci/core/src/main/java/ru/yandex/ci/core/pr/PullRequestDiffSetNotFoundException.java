package ru.yandex.ci.core.pr;

import ru.yandex.ci.core.db.EntityNotFoundException;

public class PullRequestDiffSetNotFoundException extends EntityNotFoundException {
    public PullRequestDiffSetNotFoundException(String message) {
        super(message);
    }
}
