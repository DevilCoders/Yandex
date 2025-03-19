package api

import (
	"context"
	"strings"

	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-pillar-secrets/internal/logic"
	apiv1 "a.yandex-team.ru/cloud/mdb/mdb-pillar-secrets/proto/mdb/pillarsecrets/v1"
)

func IsMDB(fullName string) bool {
	return strings.HasPrefix(fullName, "/mdb")
}

type PillarSecretService struct {
	apiv1.UnimplementedPillarSecretServiceServer

	pillarSecret logic.PillarSecret
}

var _ apiv1.PillarSecretServiceServer = &PillarSecretService{}

func NewPillarSecretService(pillarSecret logic.PillarSecret) *PillarSecretService {
	return &PillarSecretService{
		pillarSecret: pillarSecret,
	}
}

func (p *PillarSecretService) GetPillarSecret(ctx context.Context, req *apiv1.GetPillarSecretRequest) (*apiv1.GetPillarSecretResponse, error) {
	var s secret.String
	var err error

	if req.GetPath().GetKeys() == nil {
		return nil, semerr.InvalidInputf("pillar path is empty")
	}

	switch t := req.SourcePillar.(type) {
	case *apiv1.GetPillarSecretRequest_ClusterID:
		s, err = p.pillarSecret.ClusterSecret(ctx, req.GetTargetClusterID(), req.GetPath().GetKeys())
	case *apiv1.GetPillarSecretRequest_SubClusterID:
		s, err = p.pillarSecret.SubClusterSecret(ctx, req.GetTargetClusterID(), req.GetSubClusterID(), req.GetPath().GetKeys())
	case *apiv1.GetPillarSecretRequest_ShardID:
		s, err = p.pillarSecret.ShardSecret(ctx, req.GetTargetClusterID(), req.GetShardID(), req.GetPath().GetKeys())
	case *apiv1.GetPillarSecretRequest_FQDN:
		s, err = p.pillarSecret.HostSecret(ctx, req.GetTargetClusterID(), req.GetFQDN(), req.GetPath().GetKeys())
	default:
		err = semerr.InvalidInputf("invalid pillar source type %v", t)
	}

	if err != nil {
		return nil, err
	}

	return &apiv1.GetPillarSecretResponse{Value: s.Unmask()}, nil
}
