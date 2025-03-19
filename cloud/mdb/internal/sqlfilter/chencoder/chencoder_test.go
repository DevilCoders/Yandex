package chencoder_test

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlfilter"
	"a.yandex-team.ru/cloud/mdb/internal/sqlfilter/chencoder"
)

func TestEncodeFilterToClickhouseConditions(t *testing.T) {
	t.Run("Empty filter is ok", func(t *testing.T) {
		where, params, err := chencoder.EncodeFilterToClickhouseConditions("test.", nil, nil)
		require.NoError(t, err)
		require.Len(t, where, 0)
		require.Empty(t, params)
	})

	for _, testCase := range []struct {
		filter string
		where  string
	}{
		{
			"t.a = 'a' and t.b != 'b'",
			" AND a = :condVar1 AND b != :condVar2",
		},
		{
			"t.a IN ('a', 'b', 'c')",
			" AND a IN (:condVar1)",
		},
		{
			"t.a NOT IN ('a', 'b', 'c')",
			" AND a NOT IN (:condVar1)",
		},
	} {
		t.Run(testCase.filter, func(t *testing.T) {
			conds, err := sqlfilter.Parse(testCase.filter)
			require.NoError(t, err)

			where, _, err := chencoder.EncodeFilterToClickhouseConditions("t.", map[string]struct{}{"a": {}, "b": {}}, conds)
			require.NoError(t, err)
			require.Equal(t, testCase.where, where)
		})
	}

	for _, testCase := range []struct {
		name    string
		filter  string
		errText string
	}{
		{
			"not filterable column",
			"t.not_filterable = 'xxx'",
			`invalid filter field "t.not_filterable", valid: ["t.a"]`,
		},
		{
			"not filterable column",
			"ttt.a = 'xxx'",
			`invalid filter field "ttt.a", valid: ["t.a"]`,
		},
		{
			"unsupported type",
			"t.a = 42",
			`unsupported filter "t.a" field type. Should be a string or a list of strings`,
		},
		{
			"unsupported operator",
			"t.a > 'foo'",
			`unsupported filter operator > for field "t.a"`,
		},
	} {
		t.Run(fmt.Sprintf("%s: %s", testCase.name, testCase.filter), func(t *testing.T) {
			conds, err := sqlfilter.Parse(testCase.filter)
			require.NoError(t, err)

			_, _, err = chencoder.EncodeFilterToClickhouseConditions("t.", map[string]struct{}{"a": {}}, conds)
			require.Error(t, err)
			require.True(t, semerr.IsInvalidInput(err))
			require.Equal(t, testCase.errText, err.Error())
		})
	}
}
