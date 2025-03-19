package sqlbuild

import (
	"sort"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlfilter"
	"a.yandex-team.ru/cloud/mdb/internal/sqlfilter/chencoder"
	"a.yandex-team.ru/cloud/mdb/internal/stringsutil"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ColumnOptions struct {
	Filterable bool
	AggFunc    string
	Integer    bool
}

func FilterQueryColumns(columns map[string]ColumnOptions, filter []string) (map[string]ColumnOptions, error) {
	if len(filter) == 0 {
		return columns, nil
	}

	res := make(map[string]ColumnOptions, len(filter))
	for _, name := range filter {
		if opts, ok := columns[name]; !ok {
			// Translate map keys to slice and sort it for consistent output
			var valid []string
			for k := range columns {
				valid = append(valid, k)
			}
			sort.Strings(valid)
			quoted := stringsutil.QuotedJoin(valid, ", ", "\"")
			e := semerr.InvalidInputf("invalid column %q, valid columns: [%s]", name, quoted)
			return nil, xerrors.Errorf("invalid column_filter: %w", e)
		} else {
			res[name] = opts
		}
	}

	return res, nil
}

func MapJoin(columns map[string]ColumnOptions, sep string) string {
	m := make(map[string]struct{}, len(columns))
	for k := range columns {
		m[k] = struct{}{}
	}
	return stringsutil.MapJoin(m, sep)
}

// MakeQueryConditions make where condition and parameters for it.
func MakeQueryConditions(columns map[string]ColumnOptions, prefix string, conditions []sqlfilter.Term) (string, map[string]interface{}, error) {
	res := make(map[string]struct{}, len(columns))
	for c, a := range columns {
		if a.Filterable {
			res[c] = struct{}{}
		}
	}
	return chencoder.EncodeFilterToClickhouseConditions(prefix, res, conditions)
}
