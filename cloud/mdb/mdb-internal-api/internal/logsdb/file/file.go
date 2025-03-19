package file

import (
	"context"
	"io/ioutil"
	"strconv"
	"time"

	"gopkg.in/yaml.v2"

	"a.yandex-team.ru/cloud/mdb/internal/sqlfilter"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logsdb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/logs"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Backend struct {
	Path string
}

var _ logsdb.Backend = &Backend{}

func (b *Backend) IsReady(_ context.Context) error {
	return nil
}

func (b *Backend) Logs(
	_ context.Context,
	_ string,
	_ logsdb.LogType,
	_ []string,
	_, _ time.Time,
	limit, offset int64,
	_ []sqlfilter.Term,
) (res []logs.Message, more bool, err error) {
	var data []byte
	data, err = ioutil.ReadFile(b.Path)
	if err != nil {
		return
	}

	var values [][]string
	if err = yaml.Unmarshal(data, &values); err != nil {
		err = xerrors.Errorf("failed to unmarshal logsdb file data %s: %w", string(data), err)
		return
	}

	var nextPageToken int64
	// Inputs must be sorted by id
	for i, v := range values {
		// Did we fill our limit?
		if int64(len(res)) >= limit {
			// We have data left
			more = true
			return
		}

		// Ensure that nextPageToken is set to next id
		nextPageToken++

		// Are we at required offset?
		if int64(i) < offset {
			continue
		}

		if len(v) != 3 {
			err = xerrors.Errorf("expected log value length of 3 but has %d", len(v))
			return
		}

		var timeOffset int64
		timeOffset, err = strconv.ParseInt(v[1], 10, 64)
		if err != nil {
			err = xerrors.Errorf("failed to parse time offset %s: %w", v[1], err)
			return
		}

		res = append(
			res,
			logs.Message{
				// Current tests use Epoch time as starting point so use it as a base
				Timestamp:        time.Unix(0, 0).Add(time.Millisecond * time.Duration(timeOffset)),
				Message:          map[string]string{"message": v[2]},
				NextMessageToken: nextPageToken,
			},
		)
	}
	return
}
