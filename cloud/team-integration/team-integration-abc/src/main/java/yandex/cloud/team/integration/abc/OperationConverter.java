package yandex.cloud.team.integration.abc;

import com.google.protobuf.Any;
import com.google.protobuf.Empty;
import com.google.rpc.DebugInfo;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.converter.AnyConverter;
import yandex.cloud.grpc.BaseOperationConverter;
import yandex.cloud.iam.operation.OperationExceptionInfo;
import yandex.cloud.model.operation.OperationError;
import yandex.cloud.model.operation.OperationMetadata;
import yandex.cloud.model.operation.OperationResponse;
import yandex.cloud.priv.team.integration.v1.PTIAS;

public class OperationConverter extends BaseOperationConverter {

    @Override
    protected Any operationMetadata(
            @NotNull OperationMetadata metadata
    ) {
        return AnyConverter.pack(metadata)
                .bindNull(() -> null)
                .bind(CreateCloudMetadata.class, it ->
                        PTIAS.CreateCloudMetadata.newBuilder()
                                .setAbcId(it.getAbcId())
                                .setAbcSlug(it.getAbcSlug())
                                .build()
                )
                .build();
    }

    @Override
    protected Any operationResponse(
            @NotNull OperationResponse response
    ) {
        return AnyConverter.pack(response)
                .bindNull(Empty::getDefaultInstance)
                .bind(
                        CreateCloudResponse.class,
                        it -> PTIAS.CreateCloudResponse.newBuilder()
                                .setCloudId(it.getCloudId())
                                .setDefaultFolderId(it.getDefaultFolderId())
                                .build()
                )
                .build();
    }

    @Override
    protected Any operationErrorDetail(
            @NotNull OperationError.Detail errorDetail
    ) {
        return AnyConverter.pack(errorDetail)
                .bind(OperationExceptionInfo.class, this::operationExceptionInfo)
                .build();
    }

    private DebugInfo operationExceptionInfo(
            @NotNull OperationExceptionInfo info
    ) {
        return DebugInfo.newBuilder()
                .setDetail(info.getDetail())
                .addAllStackEntries(info.getStackEntries())
                .build();
    }

}
