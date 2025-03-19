package yandex.cloud.dashboard.generator;

import lombok.Value;
import yandex.cloud.dashboard.model.result.dashboard.Dashboard;

/**
 * @author ssytnik
 */
@Value
public class FolderAndDashboard {
    Long folderId;
    Dashboard dashboard;
}
