package logic

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-pillar-secrets/internal/auth"
	"a.yandex-team.ru/cloud/mdb/mdb-pillar-secrets/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/mdb-pillar-secrets/internal/metadb"
)

type PillarSecret struct {
	auth   auth.Authenticator
	crypto crypto.Crypto
}

func NewPillarSecret(auth auth.Authenticator, crypto crypto.Crypto) PillarSecret {
	return PillarSecret{
		auth:   auth,
		crypto: crypto,
	}
}

func (ps PillarSecret) ClusterSecret(ctx context.Context, cid string, path []string) (secret.String, error) {
	var encrypted crypto.CryptoKey
	var err error

	if err = ps.auth.ReadOnCluster(ctx, cid, func(ctx context.Context, metadb metadb.MetaDB) error {
		encrypted, err = metadb.ClusterPillarKey(ctx, cid, path)
		return err
	}); err != nil {
		return secret.String{}, err
	}

	return ps.crypto.Decrypt(encrypted)
}

func (ps PillarSecret) SubClusterSecret(ctx context.Context, cid, subcid string, path []string) (secret.String, error) {
	var encrypted crypto.CryptoKey
	var err error

	if err = ps.auth.ReadOnCluster(ctx, cid, func(ctx context.Context, metadb metadb.MetaDB) error {
		resolvedCid, err := metadb.ClusterIDBySubClusterID(ctx, subcid)
		if err != nil {
			return err
		}

		if resolvedCid != cid {
			return semerr.FailedPreconditionf("cluster id %q not match subcluster`s cluster id %q", cid, resolvedCid)
		}

		encrypted, err = metadb.SubClusterPillarKey(ctx, subcid, path)
		return err
	}); err != nil {
		return secret.String{}, err
	}

	return ps.crypto.Decrypt(encrypted)
}

func (ps PillarSecret) ShardSecret(ctx context.Context, cid, shardid string, path []string) (secret.String, error) {
	var encrypted crypto.CryptoKey
	var err error

	if err = ps.auth.ReadOnCluster(ctx, cid, func(ctx context.Context, metadb metadb.MetaDB) error {
		resolvedCid, err := metadb.ClusterIDByShardID(ctx, shardid)
		if err != nil {
			return err
		}

		if resolvedCid != cid {
			return semerr.FailedPreconditionf("cluster id %q not match shard`s cluster id %q", cid, resolvedCid)
		}

		encrypted, err = metadb.ShardPillarKey(ctx, shardid, path)
		return err
	}); err != nil {
		return secret.String{}, err
	}

	return ps.crypto.Decrypt(encrypted)
}

func (ps PillarSecret) HostSecret(ctx context.Context, cid, fqdn string, path []string) (secret.String, error) {
	var encrypted crypto.CryptoKey
	var err error

	if err = ps.auth.ReadOnCluster(ctx, cid, func(ctx context.Context, metadb metadb.MetaDB) error {
		resolvedCid, err := metadb.ClusterIDByFQDN(ctx, fqdn)
		if err != nil {
			return err
		}

		if resolvedCid != cid {
			return semerr.FailedPreconditionf("cluster id %q not match host`s cluster id %q", cid, resolvedCid)
		}

		encrypted, err = metadb.HostPillarKey(ctx, fqdn, path)
		return err
	}); err != nil {
		return secret.String{}, err
	}

	return ps.crypto.Decrypt(encrypted)
}
