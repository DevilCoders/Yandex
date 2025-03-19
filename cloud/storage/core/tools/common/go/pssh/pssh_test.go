package pssh

import (
	"context"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"os"
	"strconv"
	"sync"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	nbs "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
	"a.yandex-team.ru/library/go/test/portmanager"
	"a.yandex-team.ru/library/go/test/yatest"
)

////////////////////////////////////////////////////////////////////////////////

type psshHandler struct {
	t                  *testing.T
	pleaseTouchYubikey bool
}

type psshRequest struct {
	Host   string `json:"host"`
	Cmd    string `json:"cmd"`
	Format string `json:"format"`
}

type psshResponse struct {
	Host       string `json:"host"`
	Stdout     string `json:"stdout"`
	Stderr     string `json:"stderr"`
	Error      string `json:"error"`
	ExitStatus int32  `json:"exit_status"`

	PleaseTouchYubikey bool `json:"please_touch_yubikey"`
}

func (h *psshHandler) ServeHTTP(w http.ResponseWriter, req *http.Request) {
	r := &psshRequest{}
	err := json.NewDecoder(req.Body).Decode(r)
	require.NoError(h.t, err)

	if r.Cmd != "echo a" {
		http.Error(w, "unknown command", http.StatusNotImplemented)
		return
	}

	resp := &psshResponse{
		Host:               r.Host,
		Stdout:             "a",
		Stderr:             "",
		Error:              "",
		ExitStatus:         0,
		PleaseTouchYubikey: h.pleaseTouchYubikey,
	}

	payload, err := json.Marshal(resp)
	require.NoError(h.t, err)

	w.Header().Set("Content-Type", "application/json")
	_, err = w.Write(payload)
	require.NoError(h.t, err)
}

////////////////////////////////////////////////////////////////////////////////

func TestPssh(t *testing.T) {
	require.NoError(t, yatest.PrepareGOPATH())
	require.NoError(t, yatest.PrepareGOCACHE())

	mock, err := yatest.BinaryPath("cloud/blockstore/tools/testing/pssh-mock/pssh-mock")
	require.NoError(t, err)

	pm := portmanager.New(t)
	port := pm.GetPort()

	s := &http.Server{
		Addr: fmt.Sprintf(":%v", port),
		Handler: &psshHandler{
			t:                  t,
			pleaseTouchYubikey: true,
		},
	}

	var wg sync.WaitGroup
	defer wg.Wait()

	wg.Add(1)
	go func() {
		defer wg.Done()
		_ = s.ListenAndServe()
	}()

	defer func() {
		err := s.Shutdown(context.TODO())
		require.NoError(t, err)
	}()

	err = os.Setenv("PSSH_MOCK_PORT", strconv.Itoa(port))
	require.NoError(t, err)

	var triggered bool

	logger := nbs.NewLog(
		log.New(os.Stdout, "", 0),
		nbs.LOG_DEBUG,
	)

	pssh := New(logger, mock, func(ctx context.Context) error {
		_ = ctx
		triggered = true
		return nil
	})

	lines, err := pssh.Run(context.TODO(), "echo a", "localhost")
	require.NoError(t, err)
	if assert.Equal(t, 1, len(lines)) {
		assert.Equal(t, "a", lines[0])
	}

	require.Equal(t, true, triggered)
}
