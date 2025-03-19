package blackboxauth

import (
	"testing"

	"a.yandex-team.ru/cloud/mdb/internal/auth/blackbox"
	"a.yandex-team.ru/cloud/mdb/internal/auth/httpauth"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func Test_OAuthScopeChecker(t *testing.T) {
	type args struct {
		scope          string
		requiredScopes []string
	}
	tests := []struct {
		name    string
		args    args
		wantErr error
	}{
		{
			name: "contains single",
			args: args{
				scope:          "direct:api-1month metrika:write direct:api-3month metrika:read fotki:write",
				requiredScopes: []string{"direct:api-1month"},
			},
			wantErr: nil,
		},
		{
			name: "contains multiple",
			args: args{
				scope:          "direct:api-1month metrika:write direct:api-3month metrika:read fotki:write",
				requiredScopes: []string{"fotki:write", "metrika:write"},
			},
			wantErr: nil,
		},
		{
			name: "doesn't contain single",
			args: args{
				scope:          "direct:api-1month metrika:write direct:api-3month metrika:read fotki:write",
				requiredScopes: []string{"mdb-secrets:write"},
			},
			wantErr: httpauth.ErrAuthNoRights,
		},
		{
			name: "doesn't contain multiple",
			args: args{
				scope:          "direct:api-1month not-mdb-secrets:write metrika:write direct:api-3month",
				requiredScopes: []string{"mdb-secrets:write", "direct:api-1month"},
			},
			wantErr: httpauth.ErrAuthNoRights,
		},
	}
	for _, tt := range tests {
		checker := OAuthScopeListChecker{
			scopes: tt.args.requiredScopes,
		}
		info := blackbox.UserInfo{
			DBFields:    nil,
			DisplayName: blackbox.DisplayName{Name: ""},
			Login:       "",
			UID:         "",
			Scope:       tt.args.scope,
		}
		t.Run(tt.name, func(t *testing.T) {
			if err := checker.CheckScope(info); !xerrors.Is(err, tt.wantErr) {
				t.Errorf("OAuthScopeChecker error = %v, wantErr %v", err, tt.wantErr)
			}
		})
	}
}
