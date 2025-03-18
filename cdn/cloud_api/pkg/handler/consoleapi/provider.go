package consoleapi

import (
	"context"

	"google.golang.org/protobuf/types/known/emptypb"

	cdnpb "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/cdn/v1/console"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

// TODO: at the moment it is a stub

type ProviderServiceHandler struct {
	Logger log.Logger
}

func (s *ProviderServiceHandler) Activate(
	ctx context.Context,
	request *cdnpb.ActivateProviderRequest,
) (*emptypb.Empty, error) {
	ctxlog.Info(
		ctx, s.Logger,
		"activate provider request",
		log.String("folder_id", request.GetFolderId()),
		log.String("provider", request.GetProvider().String()),
	)

	return &emptypb.Empty{}, nil
}

func (s *ProviderServiceHandler) ListActivated(
	ctx context.Context,
	request *cdnpb.ListActivatedProvidersRequest,
) (*cdnpb.ListActivatedProvidersResponse, error) {
	ctxlog.Info(ctx, s.Logger, "list activated providers request", log.String("folder_id", request.GetFolderId()))

	providers := &cdnpb.ListActivatedProvidersResponse{
		Providers: []cdnpb.ProviderType{
			cdnpb.ProviderType_PROVIDER_TYPE_UNSPECIFIED,
		},
	}
	return providers, nil
}
