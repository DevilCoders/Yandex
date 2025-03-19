package ydblock

import (
	"reflect"
	"testing"
)

func Test_lockTypesFor(t *testing.T) {
	type args struct {
		lockType HostLockType
	}
	tests := []struct {
		name string
		args args
		want []HostLockType
	}{
		{"add-hosts", args{lockType: HostLockTypeAddHosts}, HostLockTypeAll},
		{"other types", args{lockType: HostLockTypeOther}, []HostLockType{HostLockTypeOther}},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			if got := lockTypesFor(tt.args.lockType); !reflect.DeepEqual(got, tt.want) {
				t.Errorf("lockTypesFor() = %v, want %v", got, tt.want)
			}
		})
	}
}
