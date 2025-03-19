package http_test

import (
	"context"
	"io/ioutil"
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/notifier"
	notifierhttp "a.yandex-team.ru/cloud/mdb/internal/notifier/http"
	"a.yandex-team.ru/library/go/core/log/nop"
)

func TestNotifyCloud(t *testing.T) {
	ts := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		assert.True(t, strings.HasSuffix(r.URL.Path, "/v1/send"))
		assert.Equal(t, "application/json", r.Header.Get("Content-Type"))
		body, err := ioutil.ReadAll(r.Body)
		assert.NoError(t, err)
		assert.JSONEq(t,
			`{
				"cloudId": "unknownCloud",
				"type": "mdb.maintenance.schedule",
				"transports": ["mail"],
				"data": {"info": "Clickhouse minor update", "delayedUntil": "2020-05-01T01:00:11Z"}
			}`, string(body))
	}))
	defer ts.Close()

	cfg := notifierhttp.Config{Endpoint: ts.URL}
	notifyAPI, err := notifierhttp.NewAPI(cfg, &nop.Logger{})
	require.NoError(t, err)
	err = notifyAPI.NotifyCloud(context.Background(), "unknownCloud", notifier.MaintenanceScheduleTemplate,
		map[string]interface{}{"info": "Clickhouse minor update", "delayedUntil": "2020-05-01T01:00:11Z"},
		[]notifier.TransportID{notifier.TransportMail})
	require.NoError(t, err)
}

func TestNotifyUser(t *testing.T) {
	ts := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		assert.True(t, strings.HasSuffix(r.URL.Path, "/v1/send"))
		assert.Equal(t, "application/json", r.Header.Get("Content-Type"))
		body, err := ioutil.ReadAll(r.Body)
		assert.NoError(t, err)
		assert.JSONEq(t,
			`{
				"userId": "unknownUser",
				"type": "mdb.maintenance.schedule",
				"transports": ["mail"],
				"data": {"info": "Clickhouse minor update", "delayedUntil": "2020-05-01T01:00:11Z"}
			}`, string(body))
	}))
	defer ts.Close()

	cfg := notifierhttp.Config{Endpoint: ts.URL}
	notifyAPI, err := notifierhttp.NewAPI(cfg, &nop.Logger{})
	require.NoError(t, err)
	err = notifyAPI.NotifyUser(context.Background(), "unknownUser", notifier.MaintenanceScheduleTemplate,
		map[string]interface{}{"info": "Clickhouse minor update", "delayedUntil": "2020-05-01T01:00:11Z"},
		[]notifier.TransportID{notifier.TransportMail})
	require.NoError(t, err)
}

func TestPing(t *testing.T) {
	ts := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		assert.True(t, strings.HasSuffix(r.URL.Path, "/ping"))
		_, err := w.Write([]byte("pong"))
		assert.NoError(t, err)
	}))
	defer ts.Close()

	cfg := notifierhttp.Config{Endpoint: ts.URL}
	notifyAPI, err := notifierhttp.NewAPI(cfg, &nop.Logger{})
	require.NoError(t, err)
	err = notifyAPI.IsReady(context.Background())
	require.NoError(t, err)
}
