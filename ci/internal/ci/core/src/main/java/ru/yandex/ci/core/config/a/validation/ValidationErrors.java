package ru.yandex.ci.core.config.a.validation;

import java.util.Collection;
import java.util.LinkedHashSet;
import java.util.Set;

import lombok.AllArgsConstructor;

public class ValidationErrors {
    private final Set<String> errors = new LinkedHashSet<>();

    public void add(String message) {
        errors.add(message);
    }

    public void addAll(Collection<String> messages) {
        errors.addAll(messages);
    }

    public boolean hasErrors() {
        return !errors.isEmpty();
    }

    public Set<String> getErrors() {
        return errors;
    }

    public ErrorState state() {
        return new ErrorState(errors.size());
    }

    @AllArgsConstructor
    public class ErrorState {
        private final int errorCount;

        public boolean hasNewErrors() {
            return errors.size() > errorCount;
        }
    }

}
