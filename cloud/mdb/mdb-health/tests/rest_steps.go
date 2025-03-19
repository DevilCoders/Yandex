package tests

import (
	"encoding/base64"
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"strings"

	"github.com/DATA-DOG/godog/gherkin"

	"a.yandex-team.ru/cloud/mdb/internal/diff"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const contentType = "application/json"

func (tctx *TestContext) weGet(path string) error {
	tctx.lastResponse = httptest.NewRecorder()
	request, _ := http.NewRequest("GET", path, nil)
	tctx.handler.ServeHTTP(tctx.lastResponse, request)
	return nil
}

func (tctx *TestContext) wePost(path string, body *gherkin.DocString) error {
	tctx.lastResponse = httptest.NewRecorder()
	request, _ := http.NewRequest("POST", path, strings.NewReader(body.Content))
	request.Header.Set("Accept", contentType)
	request.Header.Set("Content-Type", contentType)
	if err := tctx.signRequest([]byte(body.Content), request); err != nil {
		return err
	}
	tctx.handler.ServeHTTP(tctx.lastResponse, request)
	return nil
}

func (tctx *TestContext) wePut(path string, body *gherkin.DocString) error {
	tctx.lastResponse = httptest.NewRecorder()
	request, _ := http.NewRequest("PUT", path, strings.NewReader(body.Content))
	request.Header.Set("Accept", contentType)
	request.Header.Set("Content-Type", contentType)
	if err := tctx.signRequest([]byte(body.Content), request); err != nil {
		return err
	}
	tctx.handler.ServeHTTP(tctx.lastResponse, request)
	tctx.hs.Run(tctx.app.ShutdownContext())
	return nil
}

func (tctx *TestContext) weGetResponseWithStatus(code int) error {
	if tctx.lastResponse.Code != code {
		return xerrors.Errorf("expected code %v but response with code %v and body: %v", code, tctx.lastResponse.Code, tctx.lastResponse.Body.String())
	}
	return nil
}

func (tctx *TestContext) weSuccessfullyPut(path string, body *gherkin.DocString) error {
	if err := tctx.wePut(path, body); err != nil {
		return err
	}
	return tctx.weGetResponseWithStatus(200)
}

func (tctx *TestContext) weGetResponseWithContent(content *gherkin.DocString) error {
	var expected interface{}
	if err := json.Unmarshal([]byte(content.Content), &expected); err != nil {
		return xerrors.Errorf("step input json unmarshal error for body %s: %w", content.Content, err)
	}

	var actual interface{}
	if err := json.Unmarshal(tctx.lastResponse.Body.Bytes(), &actual); err != nil {
		return xerrors.Errorf("response body json unmarshal error for body %s: %w", string(tctx.lastResponse.Body.String()), err)
	}

	return diff.Full(expected, actual)
}

func (tctx *TestContext) weGetAResponseThatContains(content *gherkin.DocString) error {
	var expected map[string]interface{}
	if err := json.Unmarshal([]byte(content.Content), &expected); err != nil {
		return xerrors.Errorf("step input json unmarshal error for body %s: %w", content.Content, err)
	}

	var actual map[string]interface{}
	if err := json.Unmarshal(tctx.lastResponse.Body.Bytes(), &actual); err != nil {
		return xerrors.Errorf("response body json unmarshal error for body %s: %w", string(tctx.lastResponse.Body.String()), err)
	}

	return diff.OptionalKeys(expected, actual)
}

func (tctx *TestContext) signRequest(data []byte, request *http.Request) error {
	sign, err := tctx.pk.HashAndSign(data)
	if err != nil {
		return err
	}

	request.Header.Set("X-Signature", base64.StdEncoding.EncodeToString(sign))
	return nil
}
