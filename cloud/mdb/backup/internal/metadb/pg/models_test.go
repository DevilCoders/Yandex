package pg

import (
	"encoding/json"
	"fmt"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
)

func TestErrors_Add(t *testing.T) {
	tests := []struct {
		name           string
		merrors        []metadb.Error
		expectedErrors *Errors
	}{
		{
			name:           "empty",
			merrors:        []metadb.Error{},
			expectedErrors: NewErrors(),
		},
		{
			name: "2_errors",
			merrors: []metadb.Error{
				{Err: fmt.Errorf(`msg1: err1`), IsTemp: false, Msg: "msg1"},
				{Err: fmt.Errorf(`msg2: err2`), IsTemp: true, Msg: "msg2"},
			},
			expectedErrors: &Errors{Errors: []Error{{Err: "msg1: err1", IsTemp: false}, {Err: "msg2: err2", IsTemp: true}}},
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			errs := NewErrors()
			for _, e := range tt.merrors {
				errs.Add(e)
			}
			require.Equal(t, tt.expectedErrors, errs)
		})
	}
}

func TestErrors_JSON(t *testing.T) {
	tests := []struct {
		name   string
		errors *Errors
		raw    []byte
	}{
		{
			name:   "empty",
			errors: &Errors{},
			raw:    []byte(`{"errors":null}`),
		},
		{
			name:   "2_errors",
			errors: &Errors{Errors: []Error{{Err: "msg1: err1", IsTemp: false}, {Err: "msg2: err2", IsTemp: true}}},
			raw:    []byte(`{"errors":[{"err":"msg1: err1","is_temp":false,"ts":"0001-01-01T00:00:00Z"},{"err":"msg2: err2","is_temp":true,"ts":"0001-01-01T00:00:00Z"}]}`),
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			bytes, err := tt.errors.Marshal()
			require.NoError(t, err)
			require.Equal(t, tt.raw, bytes)

			errs := NewErrors()
			err = json.Unmarshal(tt.raw, &errs)
			require.NoError(t, err)
			require.Equal(t, tt.errors, errs)
		})
	}
}
