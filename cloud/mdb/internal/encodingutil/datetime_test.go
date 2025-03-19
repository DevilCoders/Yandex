package encodingutil_test

import (
	"fmt"
	"reflect"
	"testing"
	"time"

	"github.com/stretchr/testify/require"
	yaml "gopkg.in/yaml.v2"

	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
)

func TestDateTimeFromTime(t *testing.T) {
	dt := time.Now()
	require.Equal(t, encodingutil.DateTimeFromTime(dt).Time, dt)
}

func NewDateTime(year int, month time.Month, day, hour, min, sec int) time.Time {
	return time.Date(year, month, day, hour, min, sec, 0, time.FixedZone("MSK", 3*3600))
}

var dateTestCases = []struct {
	unmarshaled   time.Time
	marshaledJSON string
	marshaledYAML string
}{
	{unmarshaled: NewDateTime(2021, time.January, 1, 14, 30, 21), marshaledJSON: "\"2021-01-01T14:30:21+03:00\"", marshaledYAML: "2021-01-01T14:30:21+03:00"},
	{unmarshaled: NewDateTime(1990, time.May, 25, 1, 0, 0), marshaledJSON: "\"1990-05-25T01:00:00+03:00\"", marshaledYAML: "1990-05-25T01:00:00+03:00"},
}

func TestDateTimeMarshalJSON(t *testing.T) {
	for i, data := range dateTestCases {
		t.Run(
			fmt.Sprintf("MarshalJSON_%d", i),
			func(t *testing.T) {
				d := encodingutil.DateTimeFromTime(data.unmarshaled)
				marshaled, err := d.MarshalJSON()
				require.NoError(t, err)
				require.Equal(t, data.marshaledJSON, string(marshaled))
			},
		)
	}
}

func TestDateTimeUnmarshalJSON(t *testing.T) {
	for i, data := range dateTestCases {
		t.Run(
			fmt.Sprintf("UnarshalJSON_%d", i),
			func(t *testing.T) {
				var d encodingutil.DateTime
				err := d.UnmarshalJSON([]byte(data.marshaledJSON))
				require.NoError(t, err)
				require.True(t, data.unmarshaled.Equal(d.Time))
			},
		)
	}
}

func TestDatetimeUnmarshalMarshaledJSON(t *testing.T) {
	for i, data := range dateTestCases {
		t.Run(
			fmt.Sprintf("UnmarshalMarshaledJSON_%d", i),
			func(t *testing.T) {
				expected := encodingutil.DateTimeFromTime(data.unmarshaled)
				marshaled, err := expected.MarshalJSON()
				require.NoError(t, err)
				var unmarshaled encodingutil.DateTime
				err = unmarshaled.UnmarshalJSON(marshaled)
				require.NoError(t, err)
				require.True(t, unmarshaled.Equal(expected.Time))
			},
		)
	}
}

func TestDateTimeMarshalYAML(t *testing.T) {
	for i, data := range dateTestCases {
		t.Run(
			fmt.Sprintf("MarshalYAML_%d", i),
			func(t *testing.T) {
				d := encodingutil.DateTimeFromTime(data.unmarshaled)
				marshaled, err := d.MarshalYAML()
				require.NoError(t, err)
				require.Equal(t, data.marshaledYAML, marshaled.(string))
			},
		)
	}
}

func TestDatetimeMarshalWithYAMLPackage(t *testing.T) {
	for i, data := range dateTestCases {
		t.Run(
			fmt.Sprintf("MarshalWithYAMLPackage_%d", i),
			func(t *testing.T) {
				d := encodingutil.DateTimeFromTime(data.unmarshaled)
				marshaled, err := yaml.Marshal(d)
				require.NoError(t, err)
				require.Equal(t, fmt.Sprintf("\"%s\"\n", data.marshaledYAML), string(marshaled))
			},
		)
	}
}

func TestDateTimeUnmarshalYAML(t *testing.T) {
	for i, data := range dateTestCases {
		t.Run(
			fmt.Sprintf("UnarshalYAML_%d", i),
			func(t *testing.T) {
				var d encodingutil.DateTime
				err := d.UnmarshalYAML(func(v interface{}) error {
					reflect.Indirect(reflect.ValueOf(v)).Set(reflect.Indirect(reflect.ValueOf(data.marshaledYAML)))
					return nil
				})
				require.NoError(t, err)
				require.True(t, data.unmarshaled.Equal(d.Time))
			},
		)
	}
}

func TestDatetimeUnmarshalMarshaledYAML(t *testing.T) {
	for i, data := range dateTestCases {
		t.Run(
			fmt.Sprintf("UnmarshalMarshaledYAML_%d", i),
			func(t *testing.T) {
				expected := encodingutil.DateTimeFromTime(data.unmarshaled)
				marshaled, err := expected.MarshalYAML()
				require.NoError(t, err)
				var unmarshaled encodingutil.DateTime
				err = unmarshaled.UnmarshalYAML(func(v interface{}) error {
					reflect.Indirect(reflect.ValueOf(v)).Set(reflect.Indirect(reflect.ValueOf(marshaled)))
					return nil
				})
				require.NoError(t, err)
				require.True(t, unmarshaled.Equal(expected.Time))
			},
		)
	}
}
