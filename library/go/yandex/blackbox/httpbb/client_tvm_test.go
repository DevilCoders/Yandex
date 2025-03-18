package httpbb_test

import (
	"context"
	"net/http"
	"net/http/httptest"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/yandex/blackbox"
	"a.yandex-team.ru/library/go/yandex/blackbox/httpbb"
	"a.yandex-team.ru/library/go/yandex/tvm"
)

type mockTVM struct {
	tvm.Client
}

func (t *mockTVM) GetServiceTicketForID(ctx context.Context, dstID tvm.ClientID) (string, error) {
	return "ticket", nil
}

func TestClient_TVM(t *testing.T) {
	srv := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		assert.Equal(
			t,
			"ticket", r.Header.Get("X-Ya-Service-Ticket"),
			"no TVM in method: %s", r.URL.Query().Get("method"),
		)
		_, _ = w.Write([]byte(`{"status": {"id": 0,"value": "VALID"}}`))
	}))
	defer srv.Close()

	bbClient, err := httpbb.NewClient(
		httpbb.Environment{BlackboxHost: srv.URL},
		httpbb.WithTVM(&mockTVM{}),
	)
	require.NoError(t, err)

	t.Run("pass_tvm", func(t *testing.T) {
		t.Run("sessid", func(t *testing.T) {
			t.Parallel()

			_, err := bbClient.SessionID(context.Background(), blackbox.SessionIDRequest{
				SessionID: "test",
				UserIP:    "1.1.1.1",
				Host:      "test",
			})
			require.NoError(t, err)
		})

		t.Run("multisessid", func(t *testing.T) {
			t.Parallel()

			_, err := bbClient.MultiSessionID(context.Background(), blackbox.MultiSessionIDRequest{
				SessionID: "test",
				UserIP:    "1.1.1.1",
				Host:      "test",
			})
			require.NoError(t, err)
		})

		t.Run("oauth", func(t *testing.T) {
			t.Parallel()

			_, err := bbClient.OAuth(context.Background(), blackbox.OAuthRequest{
				OAuthToken: "test",
				UserIP:     "1.1.1.1",
			})
			require.NoError(t, err)
		})

		t.Run("userinfo", func(t *testing.T) {
			t.Parallel()

			_, err := bbClient.UserInfo(context.Background(), blackbox.UserInfoRequest{
				UIDs:   []blackbox.ID{1, 2, 3},
				UserIP: "1.1.1.1",
			})
			require.NoError(t, err)
		})

		t.Run("checkip", func(t *testing.T) {
			t.Parallel()

			_, err := bbClient.CheckIP(context.Background(), blackbox.CheckIPRequest{
				IP: "1.1.1.1",
			})
			require.NoError(t, err)
		})
	})
}
