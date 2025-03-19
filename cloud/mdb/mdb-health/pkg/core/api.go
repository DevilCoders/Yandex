package core

import (
	"context"
	"encoding/hex"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sentry"
	"a.yandex-team.ru/cloud/mdb/mdb-health/internal/unhealthy"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/secretsstore"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// LoadHostsHealth returns hosts health for requested fqdns
func (gw *GeneralWard) LoadHostsHealth(ctx context.Context, fqdns []string) ([]types.HostHealth, error) {
	hh, err := gw.ds.LoadHostsHealth(ctx, fqdns)
	if err != nil {
		ctxlog.Errorf(ctx, gw.logger, "Error while loading hosts health: %s", err)
		return nil, err
	}

	return hh, nil
}

// LoadClusterHealth returns hosts health for requested fqdns
func (gw *GeneralWard) LoadClusterHealth(ctx context.Context, cid string) (types.ClusterHealth, error) {
	ch, err := gw.ds.GetClusterHealth(ctx, cid)
	if err != nil {
		ctxlog.Errorf(ctx, gw.logger, "Error while loading cluster health: %s", err)
		return types.ClusterHealth{}, err
	}

	return ch, nil
}

// GetHostNeighbours fw call to datastore
func (gw *GeneralWard) GetHostNeighbours(ctx context.Context, fqdns []string) (map[string]types.HostNeighboursInfo, error) {
	hni, err := gw.ds.GetHostNeighboursInfo(ctx, fqdns)
	if err != nil {
		ctxlog.Errorf(ctx, gw.logger, "Error while loading host neighbours info: %s", err)
		return nil, err
	}

	return hni, nil
}

// LoadUnhealthyAggregatedInfo returns hosts health for requested fqdns
func (gw *GeneralWard) LoadUnhealthyAggregatedInfo(ctx context.Context, ctype metadb.ClusterType, agg types.AggType, env *string) (unhealthy.UAInfo, error) {
	uai, err := gw.ds.LoadUnhealthyAggregatedInfo(ctx, ctype, agg)
	if err != nil {
		ctxlog.Errorf(ctx, gw.logger, "Error while loading unhealthy aggregated info: %s", err)
		return unhealthy.UAInfo{}, err
	}
	resultUAI := unhealthy.UAInfo{
		StatusRecs:     make(map[unhealthy.StatusKey]*unhealthy.UARecord),
		RWRecs:         make(map[unhealthy.RWKey]*unhealthy.UARWRecord),
		WarningGeoRecs: make(map[unhealthy.GeoKey]*unhealthy.UARecord),
	}
	if env != nil && *env != "" {
		for k, v := range uai.StatusRecs {
			if k.Env == *env {
				resultUAI.StatusRecs[k] = v
			}
		}
		for k, v := range uai.WarningGeoRecs {
			if k.Env == *env {
				resultUAI.WarningGeoRecs[k] = v
			}
		}
		for k, v := range uai.RWRecs {
			if k.Env == *env {
				resultUAI.RWRecs[k] = v
			}
		}
	} else {
		for k, v := range uai.StatusRecs {
			nk := k
			nk.Env = ""
			nv, ok := resultUAI.StatusRecs[nk]
			if !ok {
				nv = &unhealthy.UARecord{}
				*nv = *v
				resultUAI.StatusRecs[nk] = nv
			} else {
				nv.Count += v.Count
				nv.Examples = append(nv.Examples, v.Examples...)
			}
		}
		for k, v := range uai.WarningGeoRecs {
			nk := k
			nk.Env = ""
			nv, ok := resultUAI.WarningGeoRecs[nk]
			if !ok {
				nv = &unhealthy.UARecord{}
				*nv = *v
				resultUAI.WarningGeoRecs[nk] = nv
			} else {
				nv.Count += v.Count
				nv.Examples = append(nv.Examples, v.Examples...)
			}
		}
		for k, v := range uai.RWRecs {
			nk := k
			nk.Env = ""
			nv, ok := resultUAI.RWRecs[nk]
			if !ok {
				nv = &unhealthy.UARWRecord{}
				*nv = *v
				resultUAI.RWRecs[nk] = nv
			} else {
				nv.Count += v.Count
				nv.NoWriteCount += v.NoWriteCount
				nv.NoReadCount += v.NoReadCount
				nv.Examples = append(nv.Examples, v.Examples...)
			}
		}
	}
	return resultUAI, nil
}

func (gw *GeneralWard) setClusterKey(cid string, val []byte) {
	gw.keys.Set(cid, val, gw.SecretTimeout())
}

// getClusterKey return noauth as second result
func (gw *GeneralWard) getClusterKey(ctx context.Context, cid string) ([]byte, bool) {
	pkc := gw.keys.Get(cid)
	if pkc == nil {
		return nil, false
	}
	pkcVal := pkc.Value()
	if pkcVal == nil {
		return nil, true
	}
	pk, ok := pkcVal.([]byte)
	if !ok {
		sentry.GlobalClient().CaptureError(
			ctx,
			xerrors.Errorf("malformed cluster key value: '%v'", pkcVal),
			map[string]string{
				"cluster_id": cid,
			},
		)
		ctxlog.Errorf(ctx, gw.logger, "check your code what you put in gw.keys, for cid '%s' strange value '%v'", cid, pkcVal)
		return nil, false
	}
	return pk, false
}

func (gw *GeneralWard) loadClusterPublicKey(ctx context.Context, cid string) ([]byte, error) {
	secret, noauth := gw.getClusterKey(ctx, cid)
	if noauth {
		return nil, semerr.Authentication(authFailedErrText)
	}
	if len(secret) > 0 {
		return secret, nil
	}

	secret, err := gw.ds.LoadClusterSecret(ctx, cid)
	if err == nil {
		gw.setClusterKey(cid, secret)
		return secret, nil
	}
	if xerrors.Is(err, datastore.ErrSecretNotFound) {
		ctxlog.Infof(ctx, gw.logger, "no secret in datastorage for cid '%s'", cid)
	} else {
		ctxlog.Errorf(ctx, gw.logger, "error while loading secret for cid '%s' from datastorage: %s", cid, err)
	}

	secret, err = gw.ss.LoadClusterSecret(ctx, cid)
	if err != nil {
		if xerrors.Is(err, secretsstore.ErrSecretNotFound) {
			// protection from invalid cid not make a lot of requests to metastorage
			gw.setClusterKey(cid, nil)

			ctxlog.Warnf(ctx, gw.logger, "host health update failed to authenticate: no secret found for cid '%s'", cid)
			return nil, semerr.WrapWithAuthentication(err, authFailedErrText)
		}

		ctxlog.Errorf(ctx, gw.logger, "error while loading secret from metastorage: %s", err)
		return nil, err
	}

	gw.setClusterKey(cid, secret)
	if err = gw.ds.StoreClusterSecret(ctx, cid, secret, gw.SecretTimeout()); err != nil {
		ctxlog.Warnf(ctx, gw.logger, "error while storing secret in datastorage: %s", err)
	}

	return secret, nil
}

// VerifyClusterSignature verifies signature of data for cid
func (gw *GeneralWard) VerifyClusterSignature(ctx context.Context, cid string, data []byte, signature []byte) error {
	keyBin, err := gw.loadClusterPublicKey(ctx, cid)
	if err != nil {
		if gw.keyOverride == nil {
			return err
		}

		// Override key if it wasn't found
		keyBin = gw.keyOverride
		ctxlog.Warnf(ctx, gw.logger, "!!!!!!!!!!!!!!!!!! USING CLUSTER KEY OVERRIDE FOR CID '%s' !!!!!!!!!!!!!!!!!!", cid)
	}

	key, err := crypto.NewPublicKey(keyBin)
	if err != nil {
		ctxlog.Errorf(ctx, gw.logger, "Error while parsing cluster '%s' public key: %s", cid, err)
		return semerr.WrapWithAuthentication(err, authFailedErrText)
	}

	if err = key.HashAndVerify(data, signature); err != nil {
		ctxlog.Warnf(ctx, gw.logger,
			"Host health update failed to authenticate for cid '%s' with signature '%s': %s",
			cid,
			hex.EncodeToString(signature),
			err,
		)
		return semerr.WrapWithAuthentication(err, authFailedErrText)
	}

	return nil
}

// StoreHostHealth stores health of one host
func (gw *GeneralWard) StoreHostHealth(ctx context.Context, hh types.HostHealth, userttl time.Duration) error {
	ttl := gw.HostHealthTimeout()
	if userttl > customTTLUpperLimit {
		return semerr.InvalidInputf("custom TTL too large, expect maximum %s", customTTLUpperLimit)
	}
	if ttl < userttl {
		ttl = userttl
	}
	if err := gw.hs.StoreHostHealth(ctx, hh, ttl); err != nil {
		return err
	}

	return nil
}
