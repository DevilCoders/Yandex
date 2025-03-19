package grpcserver

import (
	"context"
	"strings"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func extractPortoFQDN(instanceID string) (string, error) {
	res := strings.Split(instanceID, ":")
	if len(res) != 2 {
		return "", xerrors.Errorf("instance ID must be \"dom0_fqdn:container_fqdn\" format, got %q", instanceID)
	}
	return res[1], nil
}

func validateInstanceID(ctx context.Context, instanceID string, mdb metadb.MetaDB, isCompute bool) (metadb.Host, error) {
	var err error
	var h metadb.Host

	if !isCompute {
		var fqdn string
		fqdn, err = extractPortoFQDN(instanceID)
		if err != nil {
			return metadb.Host{}, semerr.InvalidInputf("wrong instance ID format for porto installation: %v", err)
		}
		h, err = mdb.GetHostByFQDN(ctx, fqdn)
	} else {
		h, err = mdb.GetHostByVtypeID(ctx, instanceID)
	}

	if xerrors.Is(err, metadb.ErrDataNotFound) {
		return metadb.Host{}, semerr.NotFoundf("unknown instance id %s", instanceID)
	}
	return h, err
}
