package osmodels

import (
	"testing"

	"github.com/stretchr/testify/require"
)

func TestVersionUnmarshalJSON(t *testing.T) {
	var err error
	v := Version{}
	err = v.UnmarshalJSON([]byte(`"7.10.2"`))
	require.NoError(t, err)
	require.Equal(t, v, mustNewVersion("7.10", "7.10.2", false, false))

	err = v.UnmarshalJSON([]byte(`"70.12.21"`))
	require.NoError(t, err)
	require.Equal(t, v, mustNewVersion("70.12", "70.12.21", false, false))

	err = v.UnmarshalJSON([]byte(`"7.10"`))
	require.Error(t, err)

	err = v.UnmarshalJSON([]byte(`"7.10.100"`))
	require.Error(t, err)
}

func TestVersionComparisons(t *testing.T) {
	versionPairs := []struct {
		lhs Version
		rhs Version
	}{
		{
			mustNewVersion("LTS", "7.10.2", false, false),
			mustNewVersion("7.9", "7.9.2", false, false),
		},
		{
			mustNewVersion("LTS", "7.10.2", false, false),
			mustNewVersion("7.10", "7.10.2", false, false),
		},
		{
			mustNewVersion("LTS", "7.10.2", false, false),
			mustNewVersion("7.10", "7.10.3", false, false),
		},
		{
			mustNewVersion("LTS", "6.10.2", false, false),
			mustNewVersion("7.9", "7.9.2", false, false),
		},
	}
	operators := []string{
		"<", "<=", ">", ">=", "==",
	}
	expected := []bool{
		false,
		false,
		true,
		true,

		false,
		true,
		true,
		true,

		true,
		false,
		false,
		false,

		true,
		true,
		false,
		false,

		false,
		true,
		false,
		false,
	}
	i := 0
	for _, op := range operators {
		for _, pair := range versionPairs {
			t.Run(op, func(t *testing.T) {
				var result bool
				switch op {
				case "<":
					result = pair.lhs.Less(pair.rhs)
				case "<=":
					result = pair.lhs.LessOrEqual(pair.rhs)
				case ">":
					result = pair.lhs.Greater(pair.rhs)
				case ">=":
					result = pair.lhs.GreaterOrEqual(pair.rhs)
				case "==":
					result = pair.lhs.Equal(pair.rhs)
				}
				require.Equalf(t, expected[i], result, "Want %+v %+v %+v", pair.lhs, op, pair.rhs)
			})
			i = i + 1
		}
	}
}
