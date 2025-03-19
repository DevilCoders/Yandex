package yandex.cloud.team.integration.abc;

import lombok.Builder;
import lombok.Value;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.model.operation.OperationResponse;

@Builder
@Value
public class CreateCloudResponse implements OperationResponse {

    @NotNull String cloudId;
    @NotNull String defaultFolderId;

}
