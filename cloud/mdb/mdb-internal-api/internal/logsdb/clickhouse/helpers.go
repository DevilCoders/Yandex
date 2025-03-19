package clickhouse

import (
	"strconv"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/library/go/core/xerrors"
)

func extractInt64FromStringMap(key string, m map[string]string) (int64, error) {
	v, ok := m[key]
	if !ok {
		return 0, xerrors.Errorf("failed to find %q key in string map %+v", key, m)
	}

	i, err := strconv.ParseInt(v, 10, 64)
	if err != nil {
		return 0, xerrors.Errorf("failed to parse value %q of key %q in string map %+v", v, key, m)
	}

	delete(m, key)
	return i, nil
}

func stringMapScan(rows *sqlx.Rows) (map[string]string, error) {
	cols, err := rows.Columns()
	if err != nil {
		return nil, err
	}
	vals := make([]string, len(cols))
	ptrs := make([]interface{}, len(vals))
	for i := range vals {
		ptrs[i] = &vals[i]
	}
	err = rows.Scan(ptrs...)
	if err != nil {
		return nil, err
	}
	m := make(map[string]string, len(cols))
	for i := range cols {
		m[cols[i]] = vals[i]
	}
	return m, nil
}
