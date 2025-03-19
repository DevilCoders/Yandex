package sqlfilter_test

import (
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlfilter"
)

func TestParseEmptyFilter(t *testing.T) {
	inputs := []struct {
		Name   string
		Filter string
	}{
		{
			"Empty string",
			"",
		},
		{
			"String with spaces",
			"   ",
		},
	}
	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			terms, err := sqlfilter.Parse(input.Filter)
			require.NoError(t, err)
			require.Empty(t, terms)
		})
	}
}

func TestParseAttributes(t *testing.T) {
	inputs := []struct {
		Filter    string
		Attribute string
	}{
		{"foo = 1", "foo"},
		{"Foo=1", "Foo"},
		{"a.b.c = 1", "a.b.c"},
		{"And=10", "And"},
		{"NoT NOT IN (42)", "NoT"},
		{"true = TRUE", "true"},
		{"FALSE = false", "FALSE"},
	}
	for _, input := range inputs {
		t.Run(input.Filter, func(t *testing.T) {
			terms, err := sqlfilter.Parse(input.Filter)
			require.NoError(t, err)
			require.Len(t, terms, 1)
			require.Equal(t, input.Attribute, terms[0].Attribute)
		})
	}
}

func TestParseOperators(t *testing.T) {
	inputs := []struct {
		Filter   string
		Operator sqlfilter.OperatorType
	}{
		{"foo = 42", sqlfilter.Equals},
		{"foo > 42", sqlfilter.Greater},
		{"foo >= 42", sqlfilter.GreaterOrEquals},
		{"foo < 42", sqlfilter.Less},
		{"foo <= 42", sqlfilter.LessOrEquals},
		{"foo IN (1, 2)", sqlfilter.In},
		{"foo iN (1, 2, 3)", sqlfilter.In},
		{"foo NOT  IN  (4, 2, 2)", sqlfilter.NotIn},
		{"foo NoT    iN       (2,42)", sqlfilter.NotIn},
	}
	for _, input := range inputs {
		t.Run(input.Filter, func(t *testing.T) {
			terms, err := sqlfilter.Parse(input.Filter)
			require.NoError(t, err)
			require.Len(t, terms, 1)
			require.Equal(t, terms[0].Operator, input.Operator)
		})
	}

}

func TestParseInt(t *testing.T) {
	inputs := []struct {
		Filter string
		Value  int64
	}{
		{"F = +1000", 1000},
		{"F>-100500", -100500},
		{"F != 9223372036854775807", 9223372036854775807},
	}
	for _, input := range inputs {
		t.Run(input.Filter, func(t *testing.T) {
			terms, err := sqlfilter.Parse(input.Filter)
			require.NoError(t, err)
			require.Len(t, terms, 1)
			require.True(t, terms[0].Value.IsInt())
			require.Equal(t, input.Value, terms[0].Value.AsInt())
		})
	}

	listInputs := []struct {
		Filter string
		L      []int64
	}{
		{"F IN (1, 2, 3)", []int64{1, 2, 3}},
		{"F NOT IN (-3, 0, 222)", []int64{-3, 0, 222}},
	}
	for _, input := range listInputs {
		t.Run(input.Filter, func(t *testing.T) {
			terms, err := sqlfilter.Parse(input.Filter)
			require.NoError(t, err)
			require.Len(t, terms, 1)
			require.True(t, terms[0].Value.IsIntList())
			require.Equal(t, input.L, terms[0].Value.AsIntList())
		})
	}

	t.Run("int overflow", func(t *testing.T) {
		_, err := sqlfilter.Parse("F = 99999999999999999999999999")
		requireInvalidInputWithMessage(t, err, "filter syntax error at or near column 5: value out of range")
	})

	t.Run("List has only one type", func(t *testing.T) {
		terms, err := sqlfilter.Parse("f IN (1,2,3)")
		require.NoError(t, err)
		require.Len(t, terms, 1)

		v := terms[0].Value
		require.True(t, v.IsIntList())
		require.False(t, v.IsTimeList(), "not time list")
		require.False(t, v.IsStringList(), "not string list")
		require.False(t, v.IsBoolList(), "not bool list")
	})
}

func TestParseString(t *testing.T) {
	inputs := []struct {
		Filter string
		Value  string
	}{
		{"single_quoted = 'foo'", "foo"},
		{`double_quoted != "foo"`, "foo"},
		{`single_with_escape = 'foo\'s'`, "foo's"},
		{`double_with_escape = "foo\"s"`, "foo\"s"},
		{`multi_quotes = "'foo'"`, `'foo'`},
		{"empty_string = ''", ""},
		{"unicode_string = '🥨'", "🥨"},
	}
	for _, input := range inputs {
		t.Run(input.Filter, func(t *testing.T) {
			terms, err := sqlfilter.Parse(input.Filter)
			require.NoError(t, err)
			require.Len(t, terms, 1)
			require.True(t, terms[0].Value.IsString())
			require.Equal(t, input.Value, terms[0].Value.AsString())
		})
	}

	listInputs := []struct {
		Filter string
		L      []string
	}{
		{"l in ('foo', 'bar')", []string{"foo", "bar"}},
		{`l NOT IN ("FOO", 'b a z\'!')`, []string{"FOO", "b a z'!"}},
	}
	for _, input := range listInputs {
		t.Run(input.Filter, func(t *testing.T) {
			terms, err := sqlfilter.Parse(input.Filter)
			require.NoError(t, err)
			require.Len(t, terms, 1)
			require.True(t, terms[0].Value.IsStringList())
			require.Equal(t, input.L, terms[0].Value.AsStringList())
		})
	}
}

func TestListTypes(t *testing.T) {
	terms, err := sqlfilter.Parse("f IN ('foo')")

	require.NoError(t, err)
	require.Len(t, terms, 1)

	v := terms[0].Value
	require.True(t, v.IsStringList())

	require.False(t, v.IsTimeList(), "not time list")
	require.False(t, v.IsIntList(), "not int list")
	require.False(t, v.IsBoolList(), "not bool list")

}

func TestParseBool(t *testing.T) {
	inputs := []struct {
		Filter string
		Value  bool
	}{
		{"B = TRUE", true},
		{"b = true", true},
		{"b!= TrUe", true},
		{"f = FALSE", false},
		{"f != fAlSe", false},
	}
	for _, input := range inputs {
		t.Run(input.Filter, func(t *testing.T) {
			terms, err := sqlfilter.Parse(input.Filter)
			require.NoError(t, err)
			require.Len(t, terms, 1)
			require.True(t, terms[0].Value.IsBool(), "bool not matched")
			require.Equal(t, input.Value, terms[0].Value.AsBool())
		})
	}
	listOfBoolInput := "foo IN (true, false)"
	t.Run(listOfBoolInput, func(t *testing.T) {
		terms, err := sqlfilter.Parse(listOfBoolInput)
		require.NoError(t, err)
		require.Len(t, terms, 1)
		require.True(t, terms[0].Value.IsBoolList())
		require.Equal(t, terms[0].Value.AsBoolList(), []bool{true, false})
	})
}

func TestParseDateTime(t *testing.T) {
	inputs := []struct {
		Filter string
		Value  string
	}{
		{"t = 2020-02-02", "2020-02-02T00:00:00Z"},
		{"t = 2020-02-02T10:15", "2020-02-02T10:15:00Z"},
		{"t = 2020-02-02T10:15:07", "2020-02-02T10:15:07Z"},
		{"t = 2020-02-02T10:15:07Z", "2020-02-02T10:15:07Z"},
		{"t = 2020-02-02T10:15+03", "2020-02-02T10:15:00+03:00"},
		{"t = 2020-02-02T10:15:07+03", "2020-02-02T10:15:07+03:00"},
		{"t = 2020-02-02T10:15:07-05:10", "2020-02-02T10:15:07-05:10"},
	}
	for _, input := range inputs {
		t.Run(input.Filter, func(t *testing.T) {
			expected, expectedErr := time.Parse(time.RFC3339, input.Value)
			if expectedErr != nil {
				t.Fatalf("Test values unparsable: %s", expectedErr)
				return
			}
			terms, err := sqlfilter.Parse(input.Filter)
			require.NoError(t, err)
			require.Len(t, terms, 1)
			require.True(t, terms[0].Value.IsTime(), "should be a time")
			require.Equal(t, expected, terms[0].Value.AsTime())
		})
	}
	listOfDateInput := "foo IN (2020-10-10, 2020-11-20T10:10)"
	t.Run(listOfDateInput, func(t *testing.T) {
		terms, err := sqlfilter.Parse(listOfDateInput)
		require.NoError(t, err)
		require.Len(t, terms, 1)
		require.True(t, terms[0].Value.IsTimeList())
		require.Len(t, terms[0].Value.AsTimeList(), 2)
	})

	t.Run("Date overflow", func(t *testing.T) {
		_, err := sqlfilter.Parse("d = 2020-50-50")
		requireInvalidInput(t, err)
		require.Equal(t, "filter syntax error at or near column 5: parsing time \"2020-50-50\": month out of range", err.Error())
	})

	t.Run("Timestamp overflow", func(t *testing.T) {
		_, err := sqlfilter.Parse("ts = 2020-10-10T42:02+03")
		requireInvalidInput(t, err)
		require.Equal(t, "filter syntax error at or near column 6: parsing time \"2020-10-10T42:02+03\": hour out of range", err.Error())
	})
}

func TestParseLogicOperators(t *testing.T) {
	andInputs := []string{
		"foo = 1 AND bar > 1",
		"foo != 1 and bar <= 1",
		"f = 2    aNd   g = 3",
	}
	for _, input := range andInputs {
		t.Run(input, func(t *testing.T) {
			terms, err := sqlfilter.Parse(input)
			require.NoError(t, err)
			require.Len(t, terms, 2)
		})
	}

}

func requireInvalidInput(t *testing.T, err error) {
	require.Error(t, err)
	require.Truef(t, semerr.IsInvalidInput(err), "should be a SemanticInvalidInput. On %+v", err)
}

func requireInvalidInputWithMessage(t *testing.T, err error, message string) {
	requireInvalidInput(t, err)
	require.Equal(t, message, err.Error())
}

func TestParseList(t *testing.T) {
	badOperators := []struct {
		Filter       string
		ErrorMessage string
	}{
		{"G > 1 AND F IN 1", "filter syntax error at or near column 11: IN operator expect list value, got int"},
		{"G >= 1 AND f = (1)", "filter syntax error at or near column 12: list values require [ NOT ] IN operator"},
	}

	for _, input := range badOperators {
		t.Run(input.Filter, func(t *testing.T) {
			_, err := sqlfilter.Parse(input.Filter)
			requireInvalidInputWithMessage(t, err, input.ErrorMessage)
		})
	}
	t.Run("list with list not supported", func(t *testing.T) {
		_, err := sqlfilter.Parse("x IN (( 2 ))")
		requireInvalidInputWithMessage(t, err, "filter syntax error at or near column 6: nested list are not supported")
	})

	t.Run("list should have elements of same type", func(t *testing.T) {
		_, err := sqlfilter.Parse("x IN ('Foo', 2)")
		requireInvalidInputWithMessage(t, err, "filter syntax error at or near column 6: list items should have same type. Item 1 is int. Previous items are strings")
	})

	t.Run("empty lists not supported", func(t *testing.T) {
		_, err := sqlfilter.Parse("L IN ()")
		requireInvalidInput(t, err)
		require.Regexp(t, `filter syntax error at or near column \d: unexpected "\)"`, err.Error())
	})

	t.Run("Trailing comma in list is not supported", func(t *testing.T) {
		_, err := sqlfilter.Parse("foo IN (1,2,)")
		requireInvalidInput(t, err)
		require.Regexp(t, "filter syntax error at or near column 13: unexpected", err.Error())
	})

	t.Run("simple value is not a list", func(t *testing.T) {
		terms, err := sqlfilter.Parse("F=1")
		require.NoError(t, err)
		require.Len(t, terms, 1)
		v := terms[0].Value
		require.False(t, v.IsIntList())
		require.False(t, v.IsStringList())
		require.False(t, v.IsTimeList())
		require.False(t, v.IsBoolList())
	})

}

func TestMultilineFilter(t *testing.T) {
	terms, err := sqlfilter.Parse(`
foo = 1
AND
bar = 'BAZ'
`)
	require.NoError(t, err)
	require.Len(t, terms, 2)
}

func TestParseErrors(t *testing.T) {
	t.Run("Trailing AND", func(t *testing.T) {
		_, err := sqlfilter.Parse("foo = 1 AND bar = 2 AND")
		requireInvalidInput(t, err)
		require.Regexp(t, `filter syntax error at or near column 2\d: unexpected "<EOF>"`, err.Error())
	})

	t.Run("Unfinished filter", func(t *testing.T) {
		_, err := sqlfilter.Parse("x =")
		requireInvalidInput(t, err)
		require.Regexp(t, `filter syntax error at or near column \d: unexpected "\<EOF\>"`, err.Error())
	})

	nonExitedOperators := []string{
		"foo <> 42",
		"foo EXISTS 42",
		"foo @@ '{blah}'",
	}
	for _, fl := range nonExitedOperators {
		t.Run("Non existed operator "+fl, func(t *testing.T) {
			_, err := sqlfilter.Parse(fl)
			requireInvalidInput(t, err)
		})
	}

	t.Run("Multiline filter", func(t *testing.T) {
		_, err := sqlfilter.Parse(`
foo = 1 AND
bar //_-) 'baz'
		`)
		requireInvalidInput(t, err)
		require.Regexp(t, `filter syntax error at or near line 3 column \d:`, err.Error())
	})

	t.Run("Unexpected ,", func(t *testing.T) {
		_, err := sqlfilter.Parse("foo = 1 , bar = 2")
		requireInvalidInput(t, err)
		require.Regexp(t, `filter syntax error at or near column \d+: unexpected token ","`, err.Error())
	})

	t.Run("Unexpected unicode in value", func(t *testing.T) {
		_, err := sqlfilter.Parse("foo = 🥨")
		requireInvalidInput(t, err)
		require.Regexp(t, `filter syntax error at or near column \d+: invalid token '🥨'`, err.Error())
	})

	t.Run("Unicode in attribute name", func(t *testing.T) {
		_, err := sqlfilter.Parse("🥨 = 'better with 🍺!'")
		requireInvalidInput(t, err)
		require.Regexp(t, `filter syntax error at or near column \d+: invalid token '🥨'`, err.Error())
	})
}
