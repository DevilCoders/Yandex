package ru.yandex.ci.tools.testenv.migration;

import lombok.Builder;
import lombok.Value;

@Value
@Builder
public class RmMigrationStatus {
    String componentName;
    String status;
    String ticket;
    String trunkDb;
}
