package clusters

import (
	"context"
	"regexp"
	"strconv"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// ListHosts should be called directly from handlers of host listing
func ListHosts(ctx context.Context, operator Operator, cid string, typ clusters.Type, pageSize int64, offset int64, opts ...OperatorOption) ([]hosts.HostExtended, int64, bool, error) {
	var res []hosts.HostExtended
	var nextPageToken int64
	var more bool

	if err := operator.ReadOnCluster(ctx, cid, typ,
		func(ctx context.Context, session sessions.Session, reader Reader, cluster Cluster) error {
			var err error
			res, nextPageToken, more, err = reader.ListHosts(ctx, cid, pageSize, offset)
			return err
		},
		opts...,
	); err != nil {
		return nil, 0, false, err
	}

	return res, nextPageToken, more, nil
}

func ListAllHosts(ctx context.Context, reader Reader, cid string) ([]hosts.HostExtended, error) {
	var result []hosts.HostExtended
	var offset int64
	for {
		newHosts, _, hasMore, err := reader.ListHosts(ctx, cid, 0, offset)
		if err != nil {
			return nil, err
		}

		offset += int64(len(newHosts))
		result = append(result, newHosts...)

		if !hasMore {
			break
		}
	}

	return result, nil
}

var fqdnIndexRegexp = regexp.MustCompile(`^\S+-([0-9]+)\.\S+\.\S+$`)

func ParseIndexFromFQDN(fqdn string) (int, error) {
	match := fqdnIndexRegexp.FindStringSubmatch(fqdn)
	if len(match) != 2 {
		return 0, xerrors.Errorf("invalid match %+v", match)
	}
	index, err := strconv.Atoi(match[1])
	if err != nil {
		return 0, xerrors.Errorf("parse fqdn: %q, err %q", fqdn, err)
	}

	return index, nil
}

func MustParseIndexFromFQDN(fqdn string) int {
	index, err := ParseIndexFromFQDN(fqdn)
	if err != nil {
		panic(err)
	}

	return index
}
