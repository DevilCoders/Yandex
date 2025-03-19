package yandex.cloud.ti.tvm.grpc;

import javax.inject.Inject;

import yandex.cloud.grpc.GrpcHeaders;
import yandex.cloud.grpc.HeaderAttachingClientInterceptor;
import yandex.cloud.iam.client.tvm.TvmClient;

public class TvmClientInterceptor extends HeaderAttachingClientInterceptor {

    @Inject
    private static TvmClient tvmClient;

    public TvmClientInterceptor(
        int tvmPeerId
    ) {
        super(GrpcHeaders.X_YA_SERVICE_TICKET, () -> tvmClient.getServiceTicket(tvmPeerId));
    }

}
