package ru.yandex.ci.client.rm;

import lombok.Value;

@Value
public class RmComponent {
    int id;
    String status;

    String releaseCycle;

    public boolean isOverCi() {
        return "ci".equals(releaseCycle);
    }

    public boolean isActive() {
        return "ACTIVE".equals(status);
    }
}
