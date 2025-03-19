package combinedauth_test

import (
	"context"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/auth/httpauth"
	"a.yandex-team.ru/cloud/mdb/internal/auth/httpauth/combinedauth"
	"a.yandex-team.ru/cloud/mdb/internal/auth/httpauth/mocks"
	"a.yandex-team.ru/library/go/core/log/nop"
)

func Test_authenticator_Auth(t *testing.T) {
	t.Run("one authenticator. success auth", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		auth1 := mocks.NewMockAuthenticator(ctrl)
		auth1.EXPECT().Auth(gomock.Any(), gomock.Any()).Return(nil)
		a, err := combinedauth.New(&nop.Logger{}, auth1)
		require.NoError(t, err)
		require.NoError(t, a.Auth(context.Background(), nil))
	})
	t.Run("2 auth providers, and credentials for first provided", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		bb := mocks.NewMockAuthenticator(ctrl)
		iam := mocks.NewMockAuthenticator(ctrl)
		iam.EXPECT().Auth(gomock.Any(), gomock.Any()).Return(nil)
		a, err := combinedauth.New(&nop.Logger{}, iam, bb)

		require.NoError(t, err)
		require.NoError(t, a.Auth(context.Background(), nil))
	})
	t.Run("2 auth providers, and no credentials for first provided", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		bb := mocks.NewMockAuthenticator(ctrl)
		iam := mocks.NewMockAuthenticator(ctrl)
		iam.EXPECT().Auth(gomock.Any(), gomock.Any()).Return(httpauth.ErrNoAuthCredentials)
		bb.EXPECT().Auth(gomock.Any(), gomock.Any()).Return(nil)
		a, err := combinedauth.New(&nop.Logger{}, iam, bb)

		require.NoError(t, err)
		require.NoError(t, a.Auth(context.Background(), nil))
	})
	t.Run("error with no auth providers", func(t *testing.T) {
		_, err := combinedauth.New(&nop.Logger{})
		require.Error(t, err)
	})
}
