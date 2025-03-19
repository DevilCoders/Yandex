package json

import (
	"testing"
)

func TestParse(t *testing.T) {
	type args struct {
		data   []byte
		result interface{}
		params ParseParams
	}
	tests := []struct {
		name    string
		args    args
		wantErr bool
	}{
		{
			name: "valid json",
			args: args{
				data: []byte(`
				{
					"Value": "value",
					"Point": 0,
					"Unknown": "ignored"
				}`),
				result: &struct {
					Value string `json:"Value" valid:"required"`
					Point *int   `json:"Point" valid:"notnull"`
				}{},
				params: ParseParams{
					IgnoreUnknown: true,
				},
			},
			wantErr: false,
		},
		{
			name: "unknown field",
			args: args{
				data: []byte(`
				{
					"Value": "value",
					"Unknown": "ignored"
				}`),
				result: &struct {
					Value string `json:"Value"`
					Point *int
				}{},
				params: ParseParams{
					IgnoreUnknown: false,
				},
			},
			wantErr: true,
		},
		{
			name: "required field is missing",
			args: args{
				data: []byte(`{}`),
				result: &struct {
					Value string `json:"Value" valid:"required"`
					Point *int
				}{},
			},
			wantErr: true,
		},
		{
			name: "notnull field",
			args: args{
				data: []byte(`
				{
					"Point": null
				}`),
				result: &struct {
					Point *int `valid:"notnull"`
				}{},
			},
			wantErr: true,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			var err error
			if err = Parse(tt.args.data, tt.args.result, tt.args.params); (err != nil) != tt.wantErr {
				t.Errorf("Parse() error = %v, wantErr %v", err, tt.wantErr)
			}
			if tt.wantErr {
				if _, ok := err.(*ValidationError); !ok {
					t.Errorf("Expected error should be type of *ValidationError: %v", err)
				}
			}
		})
	}
}
