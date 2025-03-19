package functest

import (
	"encoding/json"

	"github.com/DATA-DOG/godog/gherkin"
	"github.com/golang/protobuf/jsonpb"
	"github.com/jhump/protoreflect/dynamic"
	"github.com/jhump/protoreflect/dynamic/grpcdynamic"
	"github.com/jhump/protoreflect/grpcreflect"
	reflectpb "google.golang.org/grpc/reflection/grpc_reflection_v1alpha"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/v1"
	"a.yandex-team.ru/cloud/mdb/internal/idempotence"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (tctx *testContext) weViaDataCloudAtWithData(method, service string, body *gherkin.DocString) error {
	tctx.LastAPI = APIDATACLOUD

	ctx := tctx.TC.Context()
	// Add idempotence id ONLY if it was set
	if tctx.DataCloudCtx.IdempotenceID != "" {
		ctx = idempotence.WithOutgoing(ctx, tctx.DataCloudCtx.IdempotenceID)
	}

	rc := grpcreflect.NewClient(ctx, reflectpb.NewServerReflectionClient(tctx.DataCloudCtx.RawClient))
	sd, err := rc.ResolveService(service)
	if err != nil {
		return xerrors.Errorf("failed to resolve service %q: %w", service, err)
	}

	md := sd.FindMethodByName(method)
	if md == nil {
		return xerrors.Errorf("failed to find requested method %q in service %q", method, service)
	}

	s := grpcdynamic.NewStub(tctx.DataCloudCtx.RawClient)
	req := dynamic.NewMessage(md.GetInputType())
	if err := jsonpb.UnmarshalString(body.Content, req); err != nil {
		return xerrors.Errorf("failed to unmarshal json %s into protobuf: %w", body.Content, err)
	}

	// This might be a call to logs handle. Set expectations if needed
	if err := tctx.LogsDBCtx.MaybeExpect(method, service, req); err != nil {
		return err
	}

	resp, err := s.InvokeRpc(ctx, md, req)
	tctx.DataCloudCtx.LastResponse = resp
	tctx.DataCloudCtx.LastError = err
	return nil
}

func (tctx *testContext) weViaDataCloudAtWithNamedData(method, service, name string) error {
	data, ok := tctx.RESTCtx.NamedData[name]
	if !ok {
		return xerrors.Errorf("data %q not found", name)
	}

	body := &gherkin.DocString{
		Content: data,
	}

	return tctx.weViaDataCloudAtWithData(method, service, body)
}

func (tctx *testContext) weViaDataCloudAt(method, service string) error {
	body := &gherkin.DocString{
		Content: "{}",
	}

	return tctx.weViaDataCloudAtWithData(method, service, body)

}

func (tctx *testContext) weViaDataCloudAtWithDataAndLastPageToken(method, service string, body *gherkin.DocString) error {
	if !tctx.isLastAPI(APIDATACLOUD) {
		return nil
	}
	if !isListMethodPrefix(method) {
		return xerrors.Errorf("wrong method for pagination: %q", method)
	}

	pageToken, err := extractNextPageTokenFromLastResponse(tctx.DataCloudCtx)
	if err != nil {
		return err
	}

	content, err := appendPageTokenToBodyContent(pageToken, body.Content)
	if err != nil {
		return err
	}

	body = &gherkin.DocString{
		Content: string(content),
	}

	return tctx.weViaDataCloudAtWithData(method, service, body)

}

func extractNextPageTokenFromLastResponse(ctx *GRPCContext) (string, error) {
	mr := jsonpb.Marshaler{EmitDefaults: false, OrigName: true}
	s, err := mr.MarshalToString(ctx.LastResponse)
	if err != nil {
		return "", xerrors.Errorf("failed to marshal protobuf %s to json string: %w", ctx.LastResponse.String(), err)
	}

	var lrMap map[string]interface{}
	if err = json.Unmarshal([]byte(s), &lrMap); err != nil {
		return "", xerrors.Errorf("failed to unmarshal last response map %s: %w", s, err)
	}

	npi, ok := lrMap["next_page"]
	if !ok {
		return "", xerrors.Errorf("failed to extract 'next_page' from %q, last response %s", lrMap, ctx.LastResponse)
	}

	npMap, ok := npi.(map[string]interface{})
	if !ok {
		return "", xerrors.Errorf("failed to cast to map %q, last response %s", npi, ctx.LastResponse)
	}

	ti, ok := npMap["token"]
	if !ok {
		return "", xerrors.Errorf("failed to extract 'token' from %q, last response %s", npMap, ctx.LastResponse)
	}

	pageToken, ok := ti.(string)
	if !ok {
		return "", xerrors.Errorf("failed to cast pageToken to string %q, last response %s", ti, ctx.LastResponse)
	}

	return pageToken, nil
}

func appendPageTokenToBodyContent(pageToken string, content string) ([]byte, error) {
	var m map[string]interface{}
	if err := json.Unmarshal([]byte(content), &m); err != nil {
		return nil, xerrors.Errorf("failed to unmarshal json %q into map: %w", content, err)
	}

	pi, ok := m["paging"]
	if !ok {
		return nil, xerrors.Errorf("no paging in data %q, body content %q", m, content)
	}

	pMap, ok := pi.(map[string]interface{})
	if !ok {
		return nil, xerrors.Errorf("wrong paging in data %q, body content %q", pi, content)
	}

	psi, ok := pMap["page_size"]
	if !ok {
		return nil, xerrors.Errorf("wrong page_size in data %q, body content %q", pMap, content)
	}

	pageSize, ok := psi.(float64)
	if !ok {
		return nil, xerrors.Errorf("failed to cast to int64 %v, body content %q", psi, content)
	}

	m["paging"] = datacloud.Paging{
		PageSize:  int64(pageSize),
		PageToken: pageToken,
	}

	c, err := json.Marshal(m)
	if err != nil {
		return nil, xerrors.Errorf("failed to marshal body content %q to string: %w", content, err)
	}

	return c, nil
}

func (tctx *testContext) weGetDataCloudResponseWithBody(body *gherkin.DocString) error {
	return tctx.checkGRPCResponseWithBody(tctx.DataCloudCtx, APIDATACLOUD, body, "", false)
}

func (tctx *testContext) weGetDataCloudResponseWithBodyOmitEmpty(body *gherkin.DocString) error {
	return tctx.checkGRPCResponseWithBody(tctx.DataCloudCtx, APIDATACLOUD, body, "", true)
}

func (tctx *testContext) weSaveDataCloudResponseBodyAtPathAs(path string, varname string) error {
	return tctx.saveBodyAtPathAs(tctx.DataCloudCtx, APIDATACLOUD, true, path, varname)
}

func (tctx *testContext) weGetDataCloudResponseOK() error {
	if !tctx.isLastAPI(APIDATACLOUD) {
		return nil
	}

	if tctx.DataCloudCtx.LastError != nil {
		return xerrors.Errorf("expected no error but has one: %+v", tctx.DataCloudCtx.LastError)
	}
	return nil
}

func (tctx *testContext) weGetDataCloudResponseErrorWithCodeAndMessage(code, msg string) error {
	if !tctx.isLastAPI(APIDATACLOUD) {
		return nil
	}
	expectedCode, err := codeFromString(code)
	if err != nil {
		return err
	}

	if tctx.DataCloudCtx.LastError == nil {
		return xerrors.Errorf("expected error with code %q and message %q but has no error at all", code, msg)
	}

	st, ok := status.FromError(tctx.DataCloudCtx.LastError)
	if !ok {
		return xerrors.Errorf("expected error with code %q and message %q but error is not of gRPC Status type and has %+v", code, msg, tctx.DataCloudCtx.LastError)
	}

	if st.Code() != expectedCode {
		return xerrors.Errorf("expect error with code %q but has %q instead", codeToString(expectedCode), codeToString(st.Code()))
	}

	if st.Message() != msg {
		return xerrors.Errorf("expected error with message %q but actual message is %q", msg, st.Message())
	}

	// Assert logsdb expectations if needed
	return tctx.LogsDBCtx.AssertExpectations()
}

func (tctx *testContext) weGetDataCloudResponseErrorWithCodeAndMessageAndDetailsContain(code, msg string, body *gherkin.DocString) error {
	return tctx.checkGRPCResponseErrorWithCodeAndMessageAndDetailsContain(tctx.DataCloudCtx, APIDATACLOUD, code, msg, body)
}
