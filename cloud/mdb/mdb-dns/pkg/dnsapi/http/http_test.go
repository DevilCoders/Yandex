package http_test

import (
	"bytes"
	"context"
	"io"
	"io/ioutil"
	"net/http"
	"strings"
	"testing"
	"time"

	"github.com/gofrs/uuid"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/dnsapi"
	dahttp "a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/dnsapi/http"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

const (
	testurl     = "http://test.com"
	testacc     = "bigtest"
	testtoken   = "XXX_TEST_WAS_HERE"
	testappjson = "application/json"
)

type request struct {
	method  string
	url     []byte
	headers http.Header
	body    []byte
}

type requestDumper struct {
	fatal   bool
	calls   []request
	failCid map[string]string
}

func newReqDumper() *requestDumper {
	return &requestDumper{
		fatal: false,
	}
}

func emptyBody() io.ReadCloser {
	var empty []byte
	rbody := bytes.NewReader(empty)
	return ioutil.NopCloser(rbody)
}

func httpResp409() *http.Response {
	return &http.Response{
		Status:     "409 CONFLICT",
		StatusCode: http.StatusConflict,
		Body:       emptyBody(),
	}
}

func httpResp500() *http.Response {
	return &http.Response{
		Status:     "500 FATAL",
		StatusCode: http.StatusInternalServerError,
		Body:       emptyBody(),
	}
}

func (rd *requestDumper) RoundTrip(req *http.Request) (*http.Response, error) {
	if rd.fatal {
		return httpResp500(), nil
	}
	url, err := req.URL.MarshalBinary()
	if err != nil {
		rd.fatal = true
		return httpResp500(), nil
	}
	body, err := ioutil.ReadAll(req.Body)
	if err != nil {
		rd.fatal = true
		return httpResp500(), nil
	}
	for cid := range rd.failCid {
		if bytes.Contains(body, []byte(cid)) {
			return httpResp409(), nil
		}
	}
	rd.calls = append(rd.calls, request{
		method:  req.Method,
		url:     url,
		headers: req.Header,
		body:    body,
	})
	return &http.Response{
		Status:     "200 OK",
		StatusCode: 200,
		Body:       emptyBody(),
	}, nil
}

func initHC(t *testing.T) (context.Context, *requestDumper, dnsapi.Client) {
	ctx := context.Background()
	logger, _ := zap.New(zap.KVConfig(log.DebugLevel))
	rd := newReqDumper()
	someTTL := time.Second
	dac := dnsapi.Config{
		Baseurl: testurl,
		Account: testacc,
		Token:   secret.NewString(testtoken),
	}
	dahttp.DefaultConfig()
	hc := dahttp.New(logger, rd, dac, someTTL)
	require.NotNil(t, hc)
	return ctx, rd, hc
}

func verifyBaseUpdateRequest(t *testing.T, rd *requestDumper) {
	require.False(t, rd.fatal)
	require.Equal(t, len(rd.calls), 1)
	for _, rc := range rd.calls {
		url := string(rc.url[:])
		require.Equal(t, rc.method, http.MethodPut)
		require.True(t, strings.HasPrefix(url, testurl))
		require.True(t, strings.Contains(url, testacc))
		acc, ok := rc.headers["Accept"]
		require.True(t, ok)

		require.Equal(t, len(acc), 1)
		require.Equal(t, acc[0], testappjson)
		ct, ok := rc.headers["Content-Type"]
		require.True(t, ok)

		require.Equal(t, len(ct), 1)
		require.Equal(t, ct[0], testappjson)
		tok, ok := rc.headers["X-Auth-Token"]
		require.True(t, ok)

		require.Equal(t, len(tok), 1)
		require.Equal(t, tok[0], testtoken)
		require.True(t, len(rc.body) > 0)
	}
}

func TestSetOnce(t *testing.T) {
	ctx, rd, hc := initHC(t)

	cid := uuid.Must(uuid.NewV4()).String()
	cur := uuid.Must(uuid.NewV4()).String()
	prim := uuid.Must(uuid.NewV4()).String()

	req := &dnsapi.RequestUpdate{Records: make(dnsapi.Records)}
	req.Records[cid] = dnsapi.Update{CNAMEOld: cur, CNAMENew: prim}
	recs, err := hc.UpdateRecords(ctx, req)
	require.NoError(t, err)
	require.Equal(t, 0, len(recs))
	verifyBaseUpdateRequest(t, rd)
	body := string(rd.calls[0].body[:])

	require.True(t, strings.Contains(body, "CNAME"))
	require.True(t, strings.Contains(body, cid))
	require.True(t, strings.Contains(body, prim))
}

func TestSetFew(t *testing.T) {
	ctx, rd, hc := initHC(t)

	checkLen := 3

	req := &dnsapi.RequestUpdate{Records: make(dnsapi.Records)}
	for i := 0; i < checkLen; i++ {
		cid := uuid.Must(uuid.NewV4()).String()
		cur := uuid.Must(uuid.NewV4()).String()
		prim := uuid.Must(uuid.NewV4()).String()
		req.Records[cid] = dnsapi.Update{CNAMEOld: cur, CNAMENew: prim}
	}
	recs, err := hc.UpdateRecords(ctx, req)
	require.NoError(t, err)
	require.Equal(t, 0, len(recs))
	verifyBaseUpdateRequest(t, rd)

	body := string(rd.calls[0].body[:])
	require.True(t, strings.Contains(body, "CNAME"))
	for _, info := range req.Records {
		require.True(t, strings.Contains(body, info.CNAMEOld))
		require.True(t, strings.Contains(body, info.CNAMENew))
	}
}

func TestFailFew(t *testing.T) {
	ctx, rd, hc := initHC(t)

	checkLen := 42

	req := &dnsapi.RequestUpdate{Records: make(dnsapi.Records)}
	rd.failCid = make(map[string]string, 1+int(checkLen/13))
	for i := 0; i < checkLen; i++ {
		cid := uuid.Must(uuid.NewV4()).String()
		cur := uuid.Must(uuid.NewV4()).String()
		prim := uuid.Must(uuid.NewV4()).String()
		req.Records[cid] = dnsapi.Update{CNAMEOld: cur, CNAMENew: prim}
		if i%13 == 0 {
			rd.failCid[cid] = ""
		}
	}
	recs, err := hc.UpdateRecords(ctx, req)
	require.Equal(t, 5, len(recs))
	sumLen := 0
	for _, rec := range recs {
		sumLen += len(rec.Records)
	}
	require.Equal(t, checkLen, sumLen)
	require.Error(t, err, "should be 409")

	// no update calls successful
	require.Equal(t, len(rd.calls), 0)
}
