package ru.yandex.ci.client.sandbox.api;

import lombok.EqualsAndHashCode;
import lombok.ToString;

@ToString
@EqualsAndHashCode
public class TaskSemaphoreAcquire {
    private String name;
    private Long weight;
    private Long capacity;

    public TaskSemaphoreAcquire() {
        // no-arg constructor for jackson
    }

    public String getName() {
        return name;
    }

    public TaskSemaphoreAcquire setName(String name) {
        this.name = name;
        return this;
    }

    public Long getWeight() {
        return weight;
    }

    public TaskSemaphoreAcquire setWeight(Long weight) {
        this.weight = weight;
        return this;
    }

    public Long getCapacity() {
        return capacity;
    }

    public TaskSemaphoreAcquire setCapacity(Long capacity) {
        this.capacity = capacity;
        return this;
    }
}
