package yandex.cloud.team.integration.abc;

import lombok.Value;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.model.operation.OperationMetadata;

@Value
public class CreateCloudMetadata implements OperationMetadata {

    long abcId;
    @NotNull String abcSlug;
    @NotNull String abcFolderId;

}
