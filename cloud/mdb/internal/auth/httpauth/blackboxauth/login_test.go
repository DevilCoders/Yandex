package blackboxauth

import (
	"testing"

	"a.yandex-team.ru/cloud/mdb/internal/auth/blackbox"
	"a.yandex-team.ru/cloud/mdb/internal/auth/httpauth"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func Test_LoginSortedListChecker(t *testing.T) {
	type args struct {
		login     string
		whiteList []string
	}
	tests := []struct {
		name    string
		args    args
		wantErr error
	}{
		{
			name: "contains",
			args: args{
				login:     "alice",
				whiteList: []string{"alice", "bob"},
			},
			wantErr: nil,
		},
		{
			name: "doesn't contain",
			args: args{
				login:     "clare",
				whiteList: []string{"alice", "bob"},
			},
			wantErr: httpauth.ErrAuthNoRights,
		},
	}
	for _, tt := range tests {
		checker := NewLoginSortedListChecker(tt.args.whiteList)
		info := blackbox.UserInfo{
			DisplayName: blackbox.DisplayName{Name: ""},
			Login:       tt.args.login,
		}
		t.Run(tt.name, func(t *testing.T) {
			if err := checker.Check(info); !xerrors.Is(err, tt.wantErr) {
				t.Errorf("LoginListChecker error = %v, wantErr %v", err, tt.wantErr)
			}
		})
	}
}
