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

func TestFromDuration(t *testing.T) {
	dur := time.Second
	require.Equal(t, encodingutil.FromDuration(dur).Duration, dur)
}

var jsonData = []struct {
	unmarshaled   time.Duration
	marshaledJSON string
	marshaledYAML string
}{
	{unmarshaled: time.Second, marshaledJSON: "\"1s\"", marshaledYAML: "1s"},
	{unmarshaled: 30 * time.Second, marshaledJSON: "\"30s\"", marshaledYAML: "30s"},
	{unmarshaled: time.Millisecond, marshaledJSON: "\"1ms\"", marshaledYAML: "1ms"},
	{unmarshaled: time.Hour, marshaledJSON: "\"1h0m0s\"", marshaledYAML: "1h0m0s"},
	{unmarshaled: -time.Second, marshaledJSON: "\"-1s\"", marshaledYAML: "-1s"},
}

func TestDurationMarshalJSON(t *testing.T) {
	for i, data := range jsonData {
		t.Run(
			fmt.Sprintf("MarshalJSON_%d", i),
			func(t *testing.T) {
				d := encodingutil.FromDuration(data.unmarshaled)
				marshaled, err := d.MarshalJSON()
				require.NoError(t, err)
				require.Equal(t, data.marshaledJSON, string(marshaled))
			},
		)
	}
}

func TestDurationUnmarshalJSON(t *testing.T) {
	for i, data := range jsonData {
		t.Run(
			fmt.Sprintf("UnarshalJSON_%d", i),
			func(t *testing.T) {
				var d encodingutil.Duration
				err := d.UnmarshalJSON([]byte(data.marshaledJSON))
				require.NoError(t, err)
				require.Equal(t, data.unmarshaled, d.Duration)
			},
		)
	}
}

func TestDurationUnmarshalMarshaledJSON(t *testing.T) {
	for i, data := range jsonData {
		t.Run(
			fmt.Sprintf("UnmarshalMarshaledJSON_%d", i),
			func(t *testing.T) {
				expected := encodingutil.FromDuration(data.unmarshaled)
				marshaled, err := expected.MarshalJSON()
				require.NoError(t, err)
				var unmarshaled encodingutil.Duration
				err = unmarshaled.UnmarshalJSON(marshaled)
				require.NoError(t, err)
				require.Equal(t, expected, unmarshaled)
			},
		)
	}
}

func TestDurationMarshalYAML(t *testing.T) {
	for i, data := range jsonData {
		t.Run(
			fmt.Sprintf("MarshalYAML_%d", i),
			func(t *testing.T) {
				d := encodingutil.FromDuration(data.unmarshaled)
				marshaled, err := d.MarshalYAML()
				require.NoError(t, err)
				require.Equal(t, data.marshaledYAML, marshaled.(string))
			},
		)
	}
}

func TestDurationMarshalWithYAMLPackage(t *testing.T) {
	for i, data := range jsonData {
		t.Run(
			fmt.Sprintf("MarshalWithYAMLPackage_%d", i),
			func(t *testing.T) {
				d := encodingutil.FromDuration(data.unmarshaled)
				marshaled, err := yaml.Marshal(d)
				require.NoError(t, err)
				require.Equal(t, fmt.Sprintf("%s\n", data.marshaledYAML), string(marshaled))
			},
		)
	}
}

func TestDurationUnmarshalYAML(t *testing.T) {
	for i, data := range jsonData {
		t.Run(
			fmt.Sprintf("UnmarshalYAML_%d", i),
			func(t *testing.T) {
				var d encodingutil.Duration
				err := d.UnmarshalYAML(func(v interface{}) error {
					reflect.Indirect(reflect.ValueOf(v)).Set(reflect.Indirect(reflect.ValueOf(data.marshaledYAML)))
					return nil
				})
				require.NoError(t, err)
				require.Equal(t, data.unmarshaled, d.Duration)
			},
		)
	}
}

func TestDurationUnmarshalMarshaledYAML(t *testing.T) {
	for i, data := range jsonData {
		t.Run(
			fmt.Sprintf("UnmarshalMarshaledYAML_%d", i),
			func(t *testing.T) {
				expected := encodingutil.FromDuration(data.unmarshaled)
				marshaled, err := expected.MarshalYAML()
				require.NoError(t, err)
				var unmarshaled encodingutil.Duration
				err = unmarshaled.UnmarshalYAML(func(v interface{}) error {
					reflect.Indirect(reflect.ValueOf(v)).Set(reflect.Indirect(reflect.ValueOf(marshaled)))
					return nil
				})
				require.NoError(t, err)
				require.Equal(t, expected, unmarshaled)
			},
		)
	}
}
