package ru.yandex.ci.client.sandbox.api;

import java.util.List;

import lombok.EqualsAndHashCode;
import lombok.ToString;

@ToString
@EqualsAndHashCode
public class TaskSemaphores {
    private List<TaskSemaphoreAcquire> acquires;
    private List<String> release;

    public TaskSemaphores() {
        // no-arg constructor for jackson
    }

    public List<TaskSemaphoreAcquire> getAcquires() {
        return acquires;
    }

    public TaskSemaphores setAcquires(List<TaskSemaphoreAcquire> acquires) {
        this.acquires = acquires;
        return this;
    }

    public List<String> getRelease() {
        return release;
    }

    public TaskSemaphores setRelease(List<String> release) {
        this.release = release;
        return this;
    }
}
