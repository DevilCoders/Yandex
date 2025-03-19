package rsa

import (
	"a.yandex-team.ru/cloud/mdb/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters/provider"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ClusterSecretsRSAProvider struct{}

var _ provider.ClusterSecrets = &ClusterSecretsRSAProvider{}

func New() (*ClusterSecretsRSAProvider, error) {
	return &ClusterSecretsRSAProvider{}, nil
}

func (cs *ClusterSecretsRSAProvider) Generate() ([]byte, []byte, error) {
	clusterKey, err := crypto.GeneratePrivateKey()
	if err != nil {
		return nil, nil, xerrors.Errorf("cluster crypto key not generated: %w", err)
	}

	clusterKeyEnc, err := clusterKey.EncodeToPEM()
	if err != nil {
		return nil, nil, xerrors.Errorf("cluster crypto key not encoded: %w", err)
	}

	return clusterKeyEnc, clusterKey.GetPublicKey().EncodeToPEM(), nil
}
