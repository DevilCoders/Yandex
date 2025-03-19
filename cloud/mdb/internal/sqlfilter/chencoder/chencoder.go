package chencoder

import (
	"fmt"
	"sort"
	"strings"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlfilter"
	"a.yandex-team.ru/cloud/mdb/internal/stringsutil"
)

// First template argument is column name, second is variable-id
// Replace that format with temple.Template if you need complex mapping
// Using IN and NOT IN is OKay, cause ClickHouse driver adopts slices to CSV
var operator2condition = map[sqlfilter.OperatorType]string{
	sqlfilter.Equals:    "%s = :%s",
	sqlfilter.NotEquals: "%s != :%s",
	sqlfilter.In:        "%s IN (:%s)",
	sqlfilter.NotIn:     "%s NOT IN (:%s)",
}

func invalidQueryFilter(filterFieldPrefix string, columns map[string]struct{}, cond sqlfilter.Term) error {
	var valid []string
	for k := range columns {
		valid = append(valid, filterFieldPrefix+k)
	}
	sort.Strings(valid)

	quoted := stringsutil.QuotedJoin(valid, ", ", "\"")
	return semerr.InvalidInputf("invalid filter field %q, valid: [%s]", cond.Attribute, quoted)
}

// EncodeFilterToClickhouseConditions encode filter.Terms to clickhouse query condition +parameters for them.
// Currently we support only support condition with string attributes.
func EncodeFilterToClickhouseConditions(filterFieldPrefix string, columns map[string]struct{}, conditions []sqlfilter.Term) (string, map[string]interface{}, error) {
	if len(conditions) == 0 {
		return "", nil, nil
	}

	whereParams := make(map[string]interface{})
	var condsArr []string

	for _, cond := range conditions {
		if !strings.HasPrefix(cond.Attribute, filterFieldPrefix) {
			return "", nil, invalidQueryFilter(filterFieldPrefix, columns, cond)
		}
		columnName := cond.Attribute[len(filterFieldPrefix):]
		if _, ok := columns[columnName]; !ok {
			return "", nil, invalidQueryFilter(filterFieldPrefix, columns, cond)
		}

		var condValue interface{}
		switch {
		case cond.Value.IsString():
			condValue = cond.Value.AsString()
		case cond.Value.IsStringList():
			condValue = cond.Value.AsStringList()
		default:
			return "", nil, semerr.InvalidInputf("unsupported filter %q field type. Should be a string or a list of strings", cond.Attribute)
		}

		condTemplate, ok := operator2condition[cond.Operator]
		if !ok {
			return "", nil, semerr.InvalidInputf("unsupported filter operator %s for field %q", cond.Operator, cond.Attribute)
		}

		condID := fmt.Sprintf("condVar%d", len(condsArr)+1)
		condsArr = append(condsArr, fmt.Sprintf(condTemplate, columnName, condID))
		whereParams[condID] = condValue
	}

	whereCond := " AND " + strings.Join(condsArr, " AND ")
	return whereCond, whereParams, nil
}
