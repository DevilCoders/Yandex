package functest

import (
	"encoding/json"

	"github.com/DATA-DOG/godog/gherkin"
	"github.com/PaesslerAG/jsonpath"
	"github.com/golang/protobuf/jsonpb"
	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes"
	"github.com/jhump/protoreflect/dynamic"
	"github.com/jhump/protoreflect/dynamic/grpcdynamic"
	"github.com/jhump/protoreflect/grpcreflect"
	cpb "google.golang.org/genproto/googleapis/rpc/code"
	"google.golang.org/genproto/googleapis/rpc/errdetails"
	"google.golang.org/grpc/codes"
	reflectpb "google.golang.org/grpc/reflection/grpc_reflection_v1alpha"
	"google.golang.org/grpc/status"

	cloudquota "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/quota"
	"a.yandex-team.ru/cloud/mdb/internal/diff"
	"a.yandex-team.ru/cloud/mdb/internal/idempotence"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (tctx *testContext) weViaGRPCAtWithData(method, service string, body *gherkin.DocString) error {
	if !tctx.isModeEnabled(modeFromGRPCMethod(method), APIGRPC) {
		return nil
	}

	tctx.LastAPI = APIGRPC

	ctx := tctx.TC.Context()
	// Add idempotence id ONLY if it was set
	if tctx.GRPCCtx.IdempotenceID != "" {
		ctx = idempotence.WithOutgoing(ctx, tctx.GRPCCtx.IdempotenceID)
	}

	rc := grpcreflect.NewClient(ctx, reflectpb.NewServerReflectionClient(tctx.GRPCCtx.RawClient))
	sd, err := rc.ResolveService(service)
	if err != nil {
		return xerrors.Errorf("failed to resolve service %q: %w", service, err)
	}

	md := sd.FindMethodByName(method)
	if md == nil {
		return xerrors.Errorf("failed to find requested method %q in service %q", method, service)
	}

	s := grpcdynamic.NewStub(tctx.GRPCCtx.RawClient)
	req := dynamic.NewMessage(md.GetInputType())
	if err := jsonpb.UnmarshalString(body.Content, req); err != nil {
		return xerrors.Errorf("failed to unmarshal json %s into protobuf: %w", body.Content, err)
	}

	// This might be a call to logs handle. Set expectations if needed
	if err := tctx.LogsDBCtx.MaybeExpect(method, service, req); err != nil {
		return err
	}

	resp, err := s.InvokeRpc(ctx, md, req)
	tctx.GRPCCtx.LastResponse = resp
	tctx.GRPCCtx.LastError = err
	return nil
}

func (tctx *testContext) weViaGRPCAtWithNamedData(method, service, name string) error {
	data, ok := tctx.RESTCtx.NamedData[name]
	if !ok {
		return xerrors.Errorf("data %q not found", name)
	}

	body := &gherkin.DocString{
		Content: data,
	}

	return tctx.weViaGRPCAtWithData(method, service, body)
}

func (tctx *testContext) weViaGRPCAt(method, service string) error {
	body := &gherkin.DocString{
		Content: "{}",
	}

	return tctx.weViaGRPCAtWithData(method, service, body)
}

func (tctx *testContext) weViaGRPCAtWithDataAndLastPageToken(method, service string, body *gherkin.DocString) error {
	if !tctx.isLastAPI(APIGRPC) {
		return nil
	}
	if !isListMethodPrefix(method) {
		return xerrors.Errorf("wrong method for pagination: %q", method)
	}

	// extract next page token from last response
	mr := jsonpb.Marshaler{EmitDefaults: false, OrigName: true}
	lrs, err := mr.MarshalToString(tctx.GRPCCtx.LastResponse)
	if err != nil {
		return xerrors.Errorf("failed to marshal protobuf %s to json string: %w", tctx.GRPCCtx.LastResponse.String(), err)
	}
	var lrMap map[string]interface{}
	if err = json.Unmarshal([]byte(lrs), &lrMap); err != nil {
		return xerrors.Errorf("failed to unmarshal last response map %s: %w", lrs, err)
	}
	lastPageToken, ok := lrMap["next_page_token"]
	if !ok {
		return xerrors.Errorf("failed to extract 'next_page_token' field %q, last response %s", lrMap, tctx.GRPCCtx.LastResponse)
	}

	// manually append page token to body content
	var m map[string]interface{}
	if err := json.Unmarshal([]byte(body.Content), &m); err != nil {
		return xerrors.Errorf("failed to unmarshal json %q into map: %w", body.Content, err)
	}

	m["page_token"] = lastPageToken

	content, err := json.Marshal(m)
	if err != nil {
		return xerrors.Errorf("failed to marshal body content %q to string: %w", body.Content, err)
	}

	body = &gherkin.DocString{
		Content: string(content),
	}

	return tctx.weViaGRPCAtWithData(method, service, body)
}

func (tctx *testContext) checkGRPCResponseWithBody(ctx *GRPCContext, api API, body *gherkin.DocString, path string, omitEmpty bool) error {
	if !tctx.isLastAPI(api) {
		return nil
	}

	if ctx.LastError != nil {
		return xerrors.Errorf("expected no error but has one: %+v", ctx.LastError)
	}

	// Marshal received gRPC response to JSON string
	m := jsonpb.Marshaler{EmitDefaults: !omitEmpty, OrigName: true}
	s, err := m.MarshalToString(ctx.LastResponse)
	if err != nil {
		return xerrors.Errorf("failed to marshal protobuf %s to json string: %w", ctx.LastResponse.String(), err)
	}

	// Unmarshal expected response to map
	var expected map[string]interface{}
	if err = json.Unmarshal([]byte(body.Content), &expected); err != nil {
		return xerrors.Errorf("failed to unmarshal expected value %s: %w", body.Content, err)
	}

	// Unmarshal actual response to map
	var actual map[string]interface{}
	if err = json.Unmarshal([]byte(s), &actual); err != nil {
		return xerrors.Errorf("failed to unmarshal actual value %s: %w", s, err)
	}

	if path != "" {
		actualPart, err := jsonpath.Get(path, actual)
		if err != nil {
			return xerrors.Errorf("failed get value at jsonpath %q: %w", path, err)
		}

		actualPartMap, ok := actualPart.(map[string]interface{})
		if !ok {
			return xerrors.Errorf("actual part is of invalid type %T", actualPart)
		}

		actual = actualPartMap
	}

	if err = diff.OptionalKeys(expected, actual); err != nil {
		return err
	}

	// Assert logsdb expectations if needed
	return tctx.LogsDBCtx.AssertExpectations()
}

func (tctx *testContext) weGetGRPCResponseWithBody(body *gherkin.DocString) error {
	return tctx.checkGRPCResponseWithBody(tctx.GRPCCtx, APIGRPC, body, "", false)
}

func (tctx *testContext) weGetGRPCResponseWithBodyOmitEmpty(body *gherkin.DocString) error {
	return tctx.checkGRPCResponseWithBody(tctx.GRPCCtx, APIGRPC, body, "", true)
}

func (tctx *testContext) weGetGRPCResponseOK() error {
	if !tctx.isLastAPI(APIGRPC) {
		return nil
	}

	if tctx.GRPCCtx.LastError != nil {
		return xerrors.Errorf("expected no error but has one: %+v", tctx.GRPCCtx.LastError)
	}
	return nil
}

func (tctx *testContext) grpcResponseBodyAtPathContains(path string, body *gherkin.DocString) error {
	return tctx.checkGRPCResponseWithBody(tctx.GRPCCtx, APIGRPC, body, path, true)
}

func (tctx *testContext) grpcResponseBodyAtPathEqualsTo(path string, body *gherkin.DocString) error {
	return tctx.checkGRPCResponseWithBody(tctx.GRPCCtx, APIGRPC, body, path, false)
}

func (tctx *testContext) weGetGRPCResponseWithJSONSchema(body *gherkin.DocString) error {
	if !tctx.isLastAPI(APIGRPC) {
		return nil
	}

	if tctx.GRPCCtx.LastError != nil {
		return xerrors.Errorf("expected no error but has one: %+v", tctx.GRPCCtx.LastError)
	}

	// Get expected JSON schema
	var expectedSchema map[string]interface{}
	if err := json.Unmarshal([]byte(body.Content), &expectedSchema); err != nil {
		return xerrors.Errorf("failed to unmarshal expected value %s: %w", body.Content, err)
	}

	// Get actual JSON schema
	m := jsonpb.Marshaler{EmitDefaults: true, OrigName: true}
	responseString, err := m.MarshalToString(tctx.GRPCCtx.LastResponse)
	if err != nil {
		return xerrors.Errorf("failed to marshal protobuf %s to json string: %w", tctx.GRPCCtx.LastResponse.String(), err)
	}

	var responseMap map[string]interface{}
	if err = json.Unmarshal([]byte(responseString), &responseMap); err != nil {
		return xerrors.Errorf("failed to unmarshal actual value %s: %w", responseString, err)
	}

	marshaledSchema, ok := responseMap["schema"]
	if !ok {
		return xerrors.Errorf("response has no \"schema\" field, value: %v", responseMap)
	}

	actualSchema := make(map[string]interface{})
	err = json.Unmarshal([]byte(marshaledSchema.(string)), &actualSchema)
	if err != nil {
		return err
	}

	// Compare expected with actual JSON schema
	if err = diff.OptionalKeys(expectedSchema, actualSchema); err != nil {
		return err
	}

	return nil
}

func codeToString(code codes.Code) string {
	s, ok := cpb.Code_name[int32(code)]
	if ok {
		return s
	}
	return code.String()
}

func codeFromString(s string) (codes.Code, error) {
	var code codes.Code
	if err := code.UnmarshalJSON([]byte(`"` + s + `"`)); err != nil {
		return code, xerrors.Errorf("malformed expected code: %w", err)
	}
	return code, nil
}

func (tctx *testContext) weGetGRPCResponseErrorWithCodeAndMessage(code, msg string) error {
	if !tctx.isLastAPI(APIGRPC) {
		return nil
	}
	expectedCode, err := codeFromString(code)
	if err != nil {
		return err
	}

	if tctx.GRPCCtx.LastError == nil {
		return xerrors.Errorf("expected error with code %q and message %q but has no error at all", code, msg)
	}

	st, ok := status.FromError(tctx.GRPCCtx.LastError)
	if !ok {
		return xerrors.Errorf("expected error with code %q and message %q but error is not of gRPC Status type and has %+v", code, msg, tctx.GRPCCtx.LastError)
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

// TODO: Remove this magic after when UI will be able to handle standard typeurl - https://st.yandex-team.ru/CLOUDFRONT-4923
type anyResolver struct{}

var _ jsonpb.AnyResolver = &anyResolver{}

func (ar *anyResolver) Resolve(typeURL string) (proto.Message, error) {
	switch typeURL {
	case "type.private-api.yandex-cloud.ru/quota.QuotaFailure":
		return &cloudquota.QuotaFailure{}, nil
	}

	return nil, xerrors.Errorf("unknown typeURL %q", typeURL)
}

func (tctx *testContext) checkGRPCResponseErrorWithCodeAndMessageAndDetailsContain(ctx *GRPCContext, api API, code, msg string, body *gherkin.DocString) error {
	if !tctx.isLastAPI(api) {
		return nil
	}
	expectedCode, err := codeFromString(code)
	if err != nil {
		return err
	}

	if ctx.LastError == nil {
		return xerrors.Errorf("expected error with code %q and message %q but has no error at all", code, msg)
	}

	st, ok := status.FromError(ctx.LastError)
	if !ok {
		return xerrors.Errorf("expected error with code %q and message %q but error is not of gRPC Status type and has %+v", code, msg, ctx.LastError)
	}

	if st.Code() != expectedCode {
		return xerrors.Errorf("expect error with code %q but has %q instead", codeToString(expectedCode), codeToString(st.Code()))
	}

	if st.Message() != msg {
		return xerrors.Errorf("expected error with message %q but actual message is %q", msg, st.Message())
	}

	var expected []interface{}
	for _, d := range st.Proto().Details {
		if ptypes.Is(d, &errdetails.DebugInfo{}) {
			continue
		}

		m := jsonpb.Marshaler{EmitDefaults: false, OrigName: true, AnyResolver: &anyResolver{}}
		expectedJSON, err := m.MarshalToString(d)
		if err != nil {
			return xerrors.Errorf("failed to marshal details message %+v to json string: %w", d, err)
		}

		var expectedStruct interface{}
		if err = json.Unmarshal([]byte(expectedJSON), &expectedStruct); err != nil {
			return xerrors.Errorf("failed to unmarshal expected value %s: %w", expectedJSON, err)
		}

		expected = append(expected, expectedStruct)
	}

	var actual []interface{}
	if err = json.Unmarshal([]byte(body.Content), &actual); err != nil {
		return xerrors.Errorf("failed to unmarshal actual value %s: %w", body.Content, err)
	}

	if err = diff.Full(expected, actual); err != nil {
		return err
	}

	// Assert logsdb expectations if needed
	return tctx.LogsDBCtx.AssertExpectations()
}

func (tctx *testContext) weGetGRPCResponseErrorWithCodeAndMessageAndDetailsContain(code, msg string, body *gherkin.DocString) error {
	return tctx.checkGRPCResponseErrorWithCodeAndMessageAndDetailsContain(tctx.GRPCCtx, APIGRPC, code, msg, body)
}

func (tctx *testContext) saveBodyAtPathAs(ctx *GRPCContext, api API, omitEmpty bool, path string, varname string) error {

	if !tctx.isLastAPI(api) {
		return nil
	}

	if ctx.LastError != nil {
		return xerrors.Errorf("expected no error but has one: %+v", ctx.LastError)
	}

	// Marshal received gRPC response to JSON string
	m := jsonpb.Marshaler{EmitDefaults: !omitEmpty, OrigName: true}
	s, err := m.MarshalToString(ctx.LastResponse)
	if err != nil {
		return xerrors.Errorf("failed to marshal protobuf %s to json string: %w", ctx.LastResponse.String(), err)
	}

	// Unmarshal actual response to map
	var actual map[string]interface{}
	if err = json.Unmarshal([]byte(s), &actual); err != nil {
		return xerrors.Errorf("failed to unmarshal actual value %s: %w", s, err)
	}

	actualPart, err := jsonpath.Get(path, actual)
	if err != nil {
		return xerrors.Errorf("failed get value at jsonpath %q: %w", path, err)
	}

	actualPartMap, ok := actualPart.(string)
	if !ok {
		return xerrors.Errorf("actual part is of invalid type %T", actualPart)
	}

	tctx.variables[varname] = actualPartMap
	return nil
}

func (tctx *testContext) weAllowMoveClusterBetweenClouds() error {
	tctx.setAllowMoveClusterBetweenClouds(true)

	return nil
}

func (tctx *testContext) weDisallowMoveClusterBetweenClouds() error {
	tctx.setAllowMoveClusterBetweenClouds(false)

	return nil
}

func (tctx *testContext) setAllowMoveClusterBetweenClouds(allow bool) {
	if tctx.DataCloudInternalAPI != nil {
		tctx.DataCloudInternalAPI.Config.Logic.Flags.AllowMoveBetweenClouds = allow
	}
	if tctx.InternalAPI != nil {
		tctx.InternalAPI.Config.Logic.Flags.AllowMoveBetweenClouds = allow
	}
}

func (tctx *testContext) weAllowCreateSqlserverClusters() error {
	tctx.setAllowCreateSqlserverClusters(true)

	return nil
}

func (tctx *testContext) weDisallowCreateSqlserverClusters() error {
	tctx.setAllowCreateSqlserverClusters(false)

	return nil
}

func (tctx *testContext) setAllowCreateSqlserverClusters(allow bool) {
	if tctx.LicenseMock != nil {
		tctx.LicenseMock.SetAllowCreateClusters(allow)
	}
}

func (tctx *testContext) weUseFollowingDefaultResourcesForWithRole(clusterType, hostRole string, body *gherkin.DocString) error {
	var res logic.DefaultResourcesRecord
	if err := json.Unmarshal([]byte(body.Content), &res); err != nil {
		return xerrors.Errorf("failed to unmarshal actual value %s: %w", body.Content, err)
	}

	if tctx.InternalAPI.Config.Logic.DefaultResources.ByClusterType == nil {
		tctx.InternalAPI.Config.Logic.DefaultResources.ByClusterType = map[string]map[string]logic.DefaultResourcesRecord{}
	}

	if tctx.InternalAPI.Config.Logic.DefaultResources.ByClusterType[clusterType] == nil {
		tctx.InternalAPI.Config.Logic.DefaultResources.ByClusterType[clusterType] = map[string]logic.DefaultResourcesRecord{}
	}

	tctx.InternalAPI.Config.Logic.DefaultResources.ByClusterType[clusterType][hostRole] = res

	return nil
}
