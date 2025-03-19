package functest

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"math"
	"net"
	"net/http"
	"os"
	"path"
	"strings"

	"github.com/DATA-DOG/godog/gherkin"
	"github.com/PaesslerAG/jsonpath"
	"github.com/cenkalti/backoff/v4"
	"github.com/go-resty/resty/v2"
	"github.com/santhosh-tekuri/jsonschema/v5"
	"gopkg.in/yaml.v2"

	apihelpers "a.yandex-team.ru/cloud/mdb/dbaas-internal-api-image/recipe/helpers"
	"a.yandex-team.ru/cloud/mdb/internal/diff"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/test/yatest"
)

const (
	authHeaderName        = "X-YaCloud-SubjectToken"
	idempotenceHeaderName = "Idempotency-Key"
)

func (tctx *testContext) headers(headers *gherkin.DocString) error {
	// TODO: reset in beforescenario, not in beforefeature
	tctx.RESTCtx.Headers = make(map[string]string)
	if err := json.Unmarshal([]byte(headers.Content), &tctx.RESTCtx.Headers); err != nil {
		return err
	}

	// TODO: move it to separate step when REST API dies
	if token, ok := tctx.RESTCtx.Headers[authHeaderName]; ok {
		tctx.GRPCCtx.AuthToken = token
		tctx.DataCloudCtx.AuthToken = token
	} else {
		tctx.GRPCCtx.AuthToken = ""
		tctx.DataCloudCtx.AuthToken = ""
	}

	if id, ok := tctx.RESTCtx.Headers[idempotenceHeaderName]; ok {
		tctx.GRPCCtx.IdempotenceID = id
		tctx.DataCloudCtx.IdempotenceID = id
	} else {
		tctx.GRPCCtx.IdempotenceID = ""
		tctx.DataCloudCtx.IdempotenceID = ""
	}

	return nil
}

func (tctx *testContext) defaultHeaders() error {
	return tctx.defaultHeadersWithToken("rw-token")
}

func (tctx *testContext) defaultHeadersWithToken(tokenType string) error {
	content := fmt.Sprintf(`{
    "X-YaCloud-SubjectToken": "%s",
    "Accept": "application/json",
    "Content-Type": "application/json",
    "Access-Id": "00000000-0000-0000-0000-000000000000",
    "Access-Secret": "dummy"
}`, tokenType)

	return tctx.headers(&gherkin.DocString{
		Content: content,
	})
}

func (tctx *testContext) data(name string, data *gherkin.DocString) error {
	tctx.RESTCtx.NamedData[name] = data.Content
	return nil
}

func (tctx *testContext) s3response(data *gherkin.DocString) error {
	if err := os.MkdirAll(apihelpers.MustTmpRootPath(""), 0777); err != nil {
		return err
	}

	if err := ioutil.WriteFile(apihelpers.MustTmpRootPath("s3response.json"), []byte(data.Content), 0666); err != nil {
		return err
	}

	return nil
}

func (tctx *testContext) healthResponse(data *gherkin.DocString) error {
	if err := os.MkdirAll(apihelpers.MustTmpRootPath(""), 0777); err != nil {
		return err
	}

	if err := ioutil.WriteFile(apihelpers.MustTmpRootPath("mdb-health.json"), []byte(data.Content), 0666); err != nil {
		return err
	}

	tctx.MockData.HealthResponse = healthResponse{}
	if err := json.Unmarshal([]byte(data.Content), &tctx.MockData.HealthResponse); err != nil {
		return err
	}

	return nil
}

func (tctx *testContext) dataProcHealthResponse(data *gherkin.DocString) error {
	if err := os.MkdirAll(apihelpers.MustTmpRootPath(""), 0777); err != nil {
		return err
	}

	if err := ioutil.WriteFile(apihelpers.MustTmpRootPath("dataproc_manager_cluster_health.json"), []byte(data.Content), 0666); err != nil {
		return err
	}

	// TODO: fill data for gRPC API
	return nil
}

func (tctx *testContext) healthFromDataprocAgent() error {
	if err := os.MkdirAll(apihelpers.MustTmpRootPath(""), 0777); err != nil {
		return err
	}

	response := `{"health": "ALIVE"}`
	if err := ioutil.WriteFile(apihelpers.MustTmpRootPath("dataproc_manager_cluster_health.json"), []byte(response), 0666); err != nil {
		return err
	}

	return nil
}

func (tctx *testContext) featureFlags(data *gherkin.DocString) error {
	if err := os.MkdirAll(apihelpers.MustTmpRootPath(""), 0777); err != nil {
		return err
	}

	if err := ioutil.WriteFile(apihelpers.MustTmpRootPath("cloud-feature-flags.json"), []byte(data.Content), 0666); err != nil {
		return err
	}

	tctx.MockData.FeatureFlags = nil
	if err := json.Unmarshal([]byte(data.Content), &tctx.MockData.FeatureFlags); err != nil {
		return err
	}
	return nil
}

func (tctx *testContext) wePOST(path string) error {
	return tctx.doREST(
		path,
		func(ctx context.Context, uri string) (*resty.Response, error) {
			return tctx.RESTCtx.Client.R().SetContext(ctx).SetHeaders(tctx.RESTCtx.Headers).Post(uri)
		},
	)
}

func (tctx *testContext) wePATCHWithData(path string, data *gherkin.DocString) error {
	return tctx.doREST(
		path,
		func(ctx context.Context, uri string) (*resty.Response, error) {
			return tctx.RESTCtx.Client.R().SetContext(ctx).SetHeaders(tctx.RESTCtx.Headers).SetBody(data.Content).Patch(uri)
		},
	)
}

func (tctx *testContext) wePOSTWithData(path string, data *gherkin.DocString) error {
	return tctx.doREST(
		path,
		func(ctx context.Context, uri string) (*resty.Response, error) {
			return tctx.RESTCtx.Client.R().SetContext(ctx).SetHeaders(tctx.RESTCtx.Headers).SetBody(data.Content).Post(uri)
		},
	)
}

func (tctx *testContext) wePOSTWithNamedData(path, name string) error {
	data, ok := tctx.RESTCtx.NamedData[name]
	if !ok {
		return xerrors.Errorf("data %q not found", name)
	}

	return tctx.doREST(
		path,
		func(ctx context.Context, uri string) (*resty.Response, error) {
			return tctx.RESTCtx.Client.R().SetContext(ctx).SetHeaders(tctx.RESTCtx.Headers).SetBody(data).Post(uri)
		},
	)
}

func (tctx *testContext) weViaRESTAt(method, path string) error {
	mode, err := modeFromRESTMethod(method)
	if err != nil {
		return err
	}

	if !tctx.isModeEnabled(mode, APIREST) {
		return nil
	}

	switch method {
	case "GET":
		return tctx.weGET(path)
	case "DELETE":
		return tctx.weDELETE(path)
	case "POST":
		return tctx.wePOST(path)
	default:
		return xerrors.Errorf("unsupported REST method for this step: %s", method)
	}
}

func (tctx *testContext) weViaRESTAtWithParams(method, path string, params *gherkin.DocString) error {
	mode, err := modeFromRESTMethod(method)
	if err != nil {
		return err
	}

	if !tctx.isModeEnabled(mode, APIREST) {
		return nil
	}

	switch method {
	case "GET":
		return tctx.weGETWithParams(path, params)
	default:
		return xerrors.Errorf("unsupported REST method for this step: %s", method)
	}
}

func (tctx *testContext) weViaRESTAtWithData(method, path string, data *gherkin.DocString) error {
	mode, err := modeFromRESTMethod(method)
	if err != nil {
		return err
	}

	if !tctx.isModeEnabled(mode, APIREST) {
		return nil
	}

	switch method {
	case "POST":
		return tctx.wePOSTWithData(path, data)
	case "PATCH":
		return tctx.wePATCHWithData(path, data)
	default:
		return xerrors.Errorf("unsupported REST method for this step: %s", method)
	}
}

func (tctx *testContext) weViaRESTAtWithNamedData(method, path, name string) error {
	dataRaw, ok := tctx.RESTCtx.NamedData[name]
	if !ok {
		return xerrors.Errorf("data %q not found", name)
	}

	return tctx.weViaRESTAtWithData(method, path, &gherkin.DocString{Content: dataRaw})
}

func (tctx *testContext) weViaRESTError() error {
	if tctx.isModeEnabled(ModeModify, APIREST) || tctx.isModeEnabled(ModeRead, APIREST) {
		return xerrors.Errorf("REST mode in gRPC only test")
	}

	return nil
}

func (tctx *testContext) weGET(path string) error {
	return tctx.doREST(
		path,
		func(ctx context.Context, uri string) (*resty.Response, error) {
			return tctx.RESTCtx.Client.R().SetContext(ctx).SetHeaders(tctx.RESTCtx.Headers).Get(uri)
		},
	)
}

func (tctx *testContext) weGETWithParams(path string, params *gherkin.DocString) error {
	var p map[string]string
	// We use YAML here because it can unmarshal nonstring values directly into strings
	if err := yaml.Unmarshal([]byte(params.Content), &p); err != nil {
		return xerrors.Errorf("failed to unmarshal params %s: %w", params.Content, err)
	}

	return tctx.doREST(
		path,
		func(ctx context.Context, uri string) (*resty.Response, error) {
			return tctx.RESTCtx.Client.R().SetContext(ctx).SetHeaders(tctx.RESTCtx.Headers).SetQueryParams(p).Get(uri)
		},
	)
}

func (tctx *testContext) doREST(path string, call RestCall) error {
	tctx.LastAPI = APIREST

	uri := restURIBase() + path

	return retry(
		tctx.TC.Context(),
		0,
		func() error {
			r, err := call(tctx.TC.Context(), uri)
			if err != nil {
				var e net.Error
				if xerrors.As(err, &e) {
					return err
				}

				return backoff.Permanent(err)
			}

			tctx.RESTCtx.LastResponse = r
			return nil
		},
	)
}

func (tctx *testContext) weGetResponseWithStatus(code int) error {
	if !tctx.isLastAPI(APIREST) {
		return nil
	}

	if tctx.RESTCtx.LastResponse.StatusCode() != code {
		return xerrors.Errorf(
			"response status code is %d but expected to be %d. Body: %s",
			tctx.RESTCtx.LastResponse.StatusCode(),
			code,
			tctx.RESTCtx.LastResponse.Body(),
		)
	}

	return nil
}

func removeVersionsFromMap(pillar *map[string]interface{}) {
	// removes all versions except of postgres major and edition
	data, ok := (*pillar)["data"]
	if !ok {
		return
	}
	dataMap := data.(map[string]interface{})
	versions, ok := dataMap["versions"]
	if !ok {
		return
	}
	versionsMap := versions.(map[string]interface{})
	versionKeys := make([]string, 0, len(versionsMap))
	resVersions := map[string]interface{}{}
	for key := range versionsMap {
		versionKeys = append(versionKeys, key)
	}
	delete(dataMap, "versions")
	ignoredKeys := map[string][]string{
		"postgres": {"major_version", "edition"},
		"redis":    {"major_version"},
		"mysql":    {"major_version"},
		"mongodb":  {"major_version", "edition"},
	}
	for _, component := range versionKeys {
		currentMap := versionsMap[component].(map[string]interface{})
		for key := range currentMap {
			hideFields(currentMap, ignoredKeys, component, key)
		}
		resVersions[component] = currentMap

	}
	dataMap["versions"] = resVersions
}

func hideFields(currentMap map[string]interface{}, ignoredKeys map[string][]string, component, key string) {
	if toIgnore, ok := ignoredKeys[component]; ok {
		for _, ignoredKey := range toIgnore {
			if ignoredKey == key {
				return
			}
		}
	}
	currentMap[key] = "some hidden value"
}

func (tctx *testContext) weGetResponseWithStatusAndBodyContains(code int, body *gherkin.DocString) error {
	if !tctx.isLastAPI(APIREST) {
		return nil
	}

	if err := tctx.weGetResponseWithStatus(code); err != nil {
		return err
	}

	expected := make(map[string]interface{})
	if err := json.Unmarshal([]byte(body.Content), &expected); err != nil {
		return xerrors.Errorf("step input json unmarshal error for body %s: %w", body.Content, err)
	}

	actual := make(map[string]interface{})
	if err := json.Unmarshal(tctx.RESTCtx.LastResponse.Body(), &actual); err != nil {
		return xerrors.Errorf("response body json unmarshal error for body %s: %w", string(tctx.RESTCtx.LastResponse.Body()), err)
	}

	removeVersionsFromMap(&actual)
	return diff.OptionalKeys(expected, actual)
}

func (tctx *testContext) weGetResponseWithStatusAndBodyPartiallyContains(code int, body *gherkin.DocString) error {
	if !tctx.isLastAPI(APIREST) {
		return nil
	}

	if err := tctx.weGetResponseWithStatus(code); err != nil {
		return err
	}

	expected := make(map[string]interface{})
	if err := json.Unmarshal([]byte(body.Content), &expected); err != nil {
		return xerrors.Errorf("step input json unmarshal error for body %s: %w", body.Content, err)
	}

	actual := make(map[string]interface{})
	if err := json.Unmarshal(tctx.RESTCtx.LastResponse.Body(), &actual); err != nil {
		return xerrors.Errorf("response body json unmarshal error for body %s: %w", string(tctx.RESTCtx.LastResponse.Body()), err)
	}

	removeVersionsFromMap(&actual)
	return diff.OptionalKeysNested(expected, actual)
}

func (tctx *testContext) weGetResponseWithStatusAndBodyEqualsData(code int, name string) error {
	if !tctx.isLastAPI(APIREST) {
		return nil
	}

	if err := tctx.weGetResponseWithStatus(code); err != nil {
		return err
	}

	data, ok := tctx.RESTCtx.NamedData[name]
	if !ok {
		return xerrors.Errorf("data %q not found", name)
	}

	expected := make(map[string]interface{})
	if err := json.Unmarshal([]byte(data), &expected); err != nil {
		return xerrors.Errorf("step input json unmarshal error for body %s: %w", data, err)
	}

	actual := make(map[string]interface{})
	if err := json.Unmarshal(tctx.RESTCtx.LastResponse.Body(), &actual); err != nil {
		return xerrors.Errorf("response body json unmarshal error for body %s: %w", string(tctx.RESTCtx.LastResponse.Body()), err)
	}

	removeVersionsFromMap(&actual)
	return diff.OptionalKeys(expected, actual)
}

func (tctx *testContext) weGetResponseWithStatusAndBodyEqualsDataWithChanges(code int, name string, path2value *gherkin.DataTable) error {
	if !tctx.isLastAPI(APIREST) {
		return nil
	}

	if err := tctx.weGetResponseWithStatus(code); err != nil {
		return err
	}

	data, ok := tctx.RESTCtx.NamedData[name]
	if !ok {
		return xerrors.Errorf("data %q not found", name)
	}

	expected := make(map[string]interface{})
	if err := json.Unmarshal([]byte(data), &expected); err != nil {
		return xerrors.Errorf("step input json unmarshal error for body %s: %w", data, err)
	}

	actual := make(map[string]interface{})
	if err := json.Unmarshal(tctx.RESTCtx.LastResponse.Body(), &actual); err != nil {
		return xerrors.Errorf("response body json unmarshal error for body %s: %w", string(tctx.RESTCtx.LastResponse.Body()), err)
	}

	//Apply changes
	for _, row := range path2value.Rows {
		if len(row.Cells) != 2 {
			return xerrors.Errorf("expect table with 2 columns got %d", len(row.Cells))
		}
		jsPath := row.Cells[0].Value

		actualValue, err := jsonpath.Get(jsPath, actual)
		if err != nil {
			return xerrors.Errorf("there is not field on path %s in actual dataset, error %s", jsPath, err)
		}

		typedValue, err := getTypedValue(row.Cells[1].Value, actualValue)
		if err != nil {
			return xerrors.Errorf("error detecting type of value %s", err)
		}

		fillValueAtPath(expected, jsPath, typedValue)
	}

	removeVersionsFromMap(&actual)
	return diff.OptionalKeys(expected, actual)
}

func (tctx *testContext) weGetResponseWithStatusAndBodyMatchingFollowingValues(code int, path2value *gherkin.DataTable) error {
	if !tctx.isLastAPI(APIREST) {
		return nil
	}

	if err := tctx.weGetResponseWithStatus(code); err != nil {
		return err
	}

	actual := make(map[string]interface{})
	if err := json.Unmarshal(tctx.RESTCtx.LastResponse.Body(), &actual); err != nil {
		return xerrors.Errorf("response body json unmarshal error for body %s: %w", string(tctx.RESTCtx.LastResponse.Body()), err)
	}

	// we have only actual value, but we can still apply changes to it and check that nothing changed
	expected := make(map[string]interface{})
	if err := json.Unmarshal(tctx.RESTCtx.LastResponse.Body(), &expected); err != nil {
		return xerrors.Errorf("response body json unmarshal error for body %s: %w", string(tctx.RESTCtx.LastResponse.Body()), err)
	}

	for _, row := range path2value.Rows {
		if len(row.Cells) != 2 {
			return xerrors.Errorf("expect table with 2 columns got %d", len(row.Cells))
		}
		jsPath := row.Cells[0].Value

		expValue, err := jsonpath.Get(jsPath, expected)
		if err != nil {
			return xerrors.Errorf("there is not field on path %s in actual dataset, error %s", jsPath, err)
		}

		typedValue, err := getTypedValue(row.Cells[1].Value, expValue)
		if err != nil {
			return xerrors.Errorf("error detecting type of value %s", err)
		}

		fillValueAtPath(expected, jsPath, typedValue)
	}

	return diff.OptionalKeys(expected, actual)
}

func (tctx *testContext) weGetResponseWithStatusAndBodyEquals(code int, body *gherkin.DocString) error {
	if !tctx.isLastAPI(APIREST) {
		return nil
	}

	if err := tctx.weGetResponseWithStatus(code); err != nil {
		return err
	}

	var expected interface{}
	if err := json.Unmarshal([]byte(body.Content), &expected); err != nil {
		return xerrors.Errorf("step input json unmarshal error for body %s: %w", body.Content, err)
	}

	var actual interface{}
	if err := json.Unmarshal(tctx.RESTCtx.LastResponse.Body(), &actual); err != nil {
		return xerrors.Errorf("response body json unmarshal error for body %s: %w", string(tctx.RESTCtx.LastResponse.Body()), err)
	}

	return diff.Full(expected, actual)
}

func (tctx *testContext) weDELETE(path string) error {
	return tctx.doREST(
		path,
		func(ctx context.Context, uri string) (*resty.Response, error) {
			return tctx.RESTCtx.Client.R().SetContext(ctx).SetHeaders(tctx.RESTCtx.Headers).Delete(uri)
		},
	)
}

func (tctx *testContext) bodyAtPathContains(path string, body *gherkin.DocString) error {
	if !tctx.isLastAPI(APIREST) && !tctx.isLastAPI(APIPILLARCONFIG) {
		return nil
	}

	expected := make(map[string]interface{})
	if err := json.Unmarshal([]byte(body.Content), &expected); err != nil {
		return xerrors.Errorf("step input json unmarshal error for body %s: %w", body.Content, err)
	}

	actual := make(map[string]interface{})
	if err := json.Unmarshal(tctx.RESTCtx.LastResponse.Body(), &actual); err != nil {
		return xerrors.Errorf("response body json unmarshal error for body %s: %w", string(tctx.RESTCtx.LastResponse.Body()), err)
	}

	actualPart, err := jsonpath.Get(path, actual)
	if err != nil {
		return xerrors.Errorf("failed get value at jsonpath %q: %w", path, err)
	}

	actualPartMap, ok := actualPart.(map[string]interface{})
	if !ok {
		return xerrors.Errorf("actual part is of invalid type %T", actualPart)
	}

	return diff.OptionalKeys(expected, actualPartMap)
}

func (tctx *testContext) bodyAtPathContainsOnly(path string, body *gherkin.DocString) error {
	if !tctx.isLastAPI(APIREST) && !tctx.isLastAPI(APIPILLARCONFIG) {
		return nil
	}

	var expected interface{}
	if err := json.Unmarshal([]byte(body.Content), &expected); err != nil {
		return xerrors.Errorf("step input json unmarshal error for body %s: %w", body.Content, err)
	}

	expectedPartSlice, ok := expected.([]interface{})
	if !ok {
		return xerrors.Errorf("expected is of invalid type %T", expected)
	}

	var actual interface{}
	if err := json.Unmarshal(tctx.RESTCtx.LastResponse.Body(), &actual); err != nil {
		return xerrors.Errorf("response body json unmarshal error for body %s: %w", string(tctx.RESTCtx.LastResponse.Body()), err)
	}

	actualPart, err := jsonpath.Get(path, actual)
	if err != nil {
		return xerrors.Errorf("failed to get value at jsonpath %q: %w", path, err)
	}

	switch actualPartValue := actualPart.(type) {
	case []interface{}:
		for _, av := range actualPartValue {
			var found bool
			for _, ev := range expectedPartSlice {
				if av.(string) == ev.(string) {
					found = true
					break
				}
			}

			if !found {
				return xerrors.Errorf("value %q is not expected: %q", av, expectedPartSlice)
			}
		}
	case string:
		if len(expectedPartSlice) != 1 {
			return xerrors.Errorf("expected part slice %q must be of len 1 but is of len %d", expectedPartSlice, len(expectedPartSlice))
		}

		if actualPartValue != expectedPartSlice[0].(string) {
			return xerrors.Errorf("expected %q but has %q", expectedPartSlice[0].(string), actualPartValue)
		}
	}

	return nil
}

func (tctx *testContext) bodyAtPathIncludes(path string, body *gherkin.DocString) error {
	if !tctx.isLastAPI(APIREST) && !tctx.isLastAPI(APIPILLARCONFIG) {
		return nil
	}

	var expected []interface{}
	if err := json.Unmarshal([]byte(body.Content), &expected); err != nil {
		return xerrors.Errorf("step input json unmarshal error for body %s: %w", body.Content, err)
	}

	actual := make(map[string]interface{})
	if err := json.Unmarshal(tctx.RESTCtx.LastResponse.Body(), &actual); err != nil {
		return xerrors.Errorf("response body json unmarshal error for body %s: %w", string(tctx.RESTCtx.LastResponse.Body()), err)
	}

	actualPart, err := jsonpath.Get(path, actual)
	if err != nil {
		return xerrors.Errorf("failed get value at jsonpath %q: %w", path, err)
	}

	actualPartSlice, ok := actualPart.([]interface{})
	if !ok {
		return xerrors.Errorf("actual part is of invalid type %T", actualPart)
	}

	for _, expectedValue := range expected {
		var found bool
		for _, actualValue := range actualPartSlice {
			if expectedValue.(string) == actualValue.(string) {
				found = true
				break
			}
		}

		if !found {
			return xerrors.Errorf("expected value %q at path %q but found none", expectedValue, path)
		}
	}

	return nil
}

func (tctx *testContext) eachPathOnBodyEvaluatesTo(path2value *gherkin.DataTable) error {
	if !tctx.isLastAPI(APIREST) {
		return nil
	}

	actual := make(map[string]interface{})
	if err := json.Unmarshal(tctx.RESTCtx.LastResponse.Body(), &actual); err != nil {
		return xerrors.Errorf("response body json unmarshal error for body %s: %w", string(tctx.RESTCtx.LastResponse.Body()), err)
	}

	for _, row := range path2value.Rows {
		if len(row.Cells) != 2 {
			return xerrors.Errorf("expect table with 2 columns got %d", len(row.Cells))
		}
		jsPath := row.Cells[0].Value
		actualValue, err := jsonpath.Get(jsPath, actual)
		if err != nil {
			return xerrors.Errorf("failed to apply path %s: %s. Body: %s", jsPath, err, string(tctx.RESTCtx.LastResponse.Body()))
		}
		var expectedValue interface{}
		if err := json.Unmarshal([]byte(row.Cells[1].Value), &expectedValue); err != nil {
			return xerrors.Errorf("actual value '%s' unmarshal: %s", row.Cells[1].Value, err)
		}

		if err := diff.Full(expectedValue, actualValue); err != nil {
			return xerrors.Errorf("at '%s' path: %s", jsPath, err)
		}
	}
	return nil
}

func (tctx *testContext) bodyDoesNotContainsPaths(path2value *gherkin.DataTable) error {
	if !tctx.isLastAPI(APIREST) {
		return nil
	}

	actual := make(map[string]interface{})
	if err := json.Unmarshal(tctx.RESTCtx.LastResponse.Body(), &actual); err != nil {
		return xerrors.Errorf("response body json unmarshal error for body %s: %w", string(tctx.RESTCtx.LastResponse.Body()), err)
	}

	for _, row := range path2value.Rows {
		if len(row.Cells) != 1 {
			return xerrors.Errorf("expect table with 1 columns got %d", len(row.Cells))
		}
		jsPath := row.Cells[0].Value
		value, err := jsonpath.Get(jsPath, actual)
		if err != nil {

		} else {
			return xerrors.Errorf("body has value on path %s: %s. Body: %s", jsPath, value, string(tctx.RESTCtx.LastResponse.Body()))
		}
	}
	return nil
}

func (tctx *testContext) bodyAtPathContainsFieldsWithOrder(path string, order string) error {
	if !tctx.isLastAPI(APIREST) {
		return nil
	}

	// Field describes expected object key, its sort order and value in previous object of our slice
	type Field struct {
		Name    string
		Desc    bool
		Float64 optional.Float64
		String  optional.String
	}

	// Invalidates field values starting from i index
	invalidate := func(fields []*Field, i int) {
		fmt.Printf("invalidating fields starting from %d to %d\n", i, len(fields))
		for ; i < len(fields); i++ {
			fields[i].Float64.Valid = false
			fields[i].String.Valid = false
		}
	}

	splitFields := strings.Split(order, ";")
	fields := make([]*Field, 0, len(splitFields))
	for _, field := range splitFields {
		name := strings.TrimLeft(field, "-")
		desc := strings.HasPrefix(field, "-")
		fields = append(fields, &Field{Name: name, Desc: desc})
	}

	var actual interface{}
	if err := json.Unmarshal(tctx.RESTCtx.LastResponse.Body(), &actual); err != nil {
		return xerrors.Errorf("response body json unmarshal error for body %s: %w", string(tctx.RESTCtx.LastResponse.Body()), err)
	}

	actualPart, err := jsonpath.Get(path, actual)
	if err != nil {
		return xerrors.Errorf("failed to get value at jsonpath %q: %w", path, err)
	}

	slice, ok := actualPart.([]interface{})
	if !ok {
		return xerrors.Errorf("actual part is of invalid type %T", actualPart)
	}

	// Walk over each object
	for _, sliceValue := range slice {
		m, ok := sliceValue.(map[string]interface{})
		if !ok {
			return xerrors.Errorf("slice value is of invalid type %T", sliceValue)
		}

		// Walk over each expected field
		for i, field := range fields {
			// Find field in object
			untypedValue, ok := m[field.Name]
			if !ok {
				return xerrors.Errorf("field %q not found in slice value %+v", field.Name, m)
			}

			// Go typed mode
			switch typedValue := untypedValue.(type) {
			case float64:
				fmt.Printf("%d %s: %f (%t) vs %f\n", i, field.Name, field.Float64.Float64, field.Float64.Valid, typedValue)

				// Do we have previous valid value?
				if field.Float64.Valid {
					// Check ordering
					if (typedValue < field.Float64.Float64 && !field.Desc) ||
						(typedValue > field.Float64.Float64 && field.Desc) {
						return xerrors.Errorf("field %q is not in correct order (%t) compared to previous value: %f vs %f", field.Name, field.Desc, typedValue, field.Float64.Float64)
					}

					// Invalidate next fields if our value changed. We don't need to invalidate if previous value is invalid (all other fields are invalid too).
					const tolerance = 0.0000001
					if math.Abs(typedValue-field.Float64.Float64) > tolerance {
						invalidate(fields, i+1)
					}
				}

				// Remember field value
				field.Float64.Set(typedValue)
			case string:
				fmt.Printf("%d %s: %s (%t) vs %s\n", i, field.Name, field.String.String, field.String.Valid, typedValue)

				// Do we have previous valid value?
				if field.String.Valid {
					// Check ordering
					if (typedValue < field.String.String && !field.Desc) ||
						(typedValue > field.String.String && field.Desc) {
						return xerrors.Errorf("field %q is not in correct order (%t) compared to previous value: %s vs %s", field.Name, field.Desc, typedValue, field.String.String)
					}

					// Invalidate next fields if our value changed. We don't need to invalidate if previous value is invalid (all other fields are invalid too).
					if typedValue != field.String.String {
						invalidate(fields, i+1)
					}
				}

				// Remember field value
				field.String.Set(typedValue)
			default:
				return xerrors.Errorf("map key %q has value %+v of invalid type %T", field.Name, untypedValue, untypedValue)
			}
		}
	}

	return nil
}

/*
type relativeFileSystemLoader struct {
	// Root is an absolute path that
	Root string
}

func (fs *relativeFileSystemLoader) Open(name string) (http.File, error) {
	fmt.Println("OPEN: ", name)
	// Root path must be absolute
	if !filepath.IsAbs(fs.Root) {
		return nil, xerrors.Errorf("root path is not absolute: %q", fs.Root)
	}

	// Load from provided path if its an absolute path
	if filepath.IsAbs(name) {
		fmt.Println("OPEN AS IS: ", name)
		return os.Open(name)
	}

	// Add root as prefix and make path absolute
	fmt.Println("OPEN JOIN: ", path.Join(fs.Root, name))
	return os.Open(path.Join(fs.Root, name))
}*/

func (tctx *testContext) bodyMatchesSchema(schema *gherkin.DocString) error {
	if !tctx.isLastAPI(APIREST) {
		return nil
	}

	schemaCompiler := jsonschema.NewCompiler()
	schemaCompiler.Draft = jsonschema.Draft2019
	err := schemaCompiler.AddResource("schema.json", strings.NewReader(schema.Content))
	if err != nil {
		return xerrors.Errorf("failed to add JSON schema to compiler: %w", err)
	}

	jsonSchema, err := schemaCompiler.Compile("schema.json")
	if err != nil {
		return xerrors.Errorf("failed to compile JSON schema: %w", err)
	}

	var v interface{}
	err = json.Unmarshal(tctx.RESTCtx.LastResponse.Body(), &v)
	if err != nil {
		tctx.L.Errorf("Input JSON: %s", string(tctx.RESTCtx.LastResponse.Body()))
		return xerrors.Errorf("failed to unmarshal JSON body: %w", err)
	}

	err = jsonSchema.Validate(v)
	if err != nil {
		tctx.L.Errorf("Input JSON: %s", string(tctx.RESTCtx.LastResponse.Body()))
		return xerrors.Errorf("failed to validate JSON against schema: %#v", err)
	}

	return nil
}

func (tctx *testContext) bodyMatchesSchemaFromFile(filename string) error {
	if !tctx.isLastAPI(APIREST) {
		return nil
	}

	filepath := path.Join(yatest.SourcePath("cloud/mdb/mdb-internal-api/functest/features/"), filename)
	schema, err := jsonschema.Compile(filepath)
	if err != nil {
		return xerrors.Errorf("failed to compile JSON schema: %w", err)
	}

	var v interface{}
	err = json.Unmarshal(tctx.RESTCtx.LastResponse.Body(), &v)
	if err != nil {
		tctx.L.Errorf("Input JSON: %s", string(tctx.RESTCtx.LastResponse.Body()))
		return xerrors.Errorf("failed to unmarshal JSON body: %w", err)
	}

	err = schema.Validate(v)
	if err != nil {
		tctx.L.Errorf("Input JSON: %s", string(tctx.RESTCtx.LastResponse.Body()))
		return xerrors.Errorf("failed to validate JSON against schema: %#v", err)
	}

	return nil
}

func (tctx *testContext) bodyIsValidJSONSchema() error {
	if !tctx.isLastAPI(APIREST) {
		return nil
	}

	rawSchema := bytes.NewReader(tctx.RESTCtx.LastResponse.Body())

	schemaCompiler := jsonschema.NewCompiler()
	schemaCompiler.Draft = jsonschema.Draft2019
	err := schemaCompiler.AddResource("schema.json", rawSchema)
	if err != nil {
		return xerrors.Errorf("failed to add JSON schema to compiler: %w", err)
	}

	_, err = schemaCompiler.Compile("schema.json")
	if err != nil {
		return xerrors.Errorf("failed to validate JSON schema: %w", err)
	}

	return nil
}

func (tctx *testContext) doPillarConfig(path string, call RestCall) error {
	tctx.LastAPI = APIPILLARCONFIG

	uri := pillarConfigURIBase() + path

	return retry(
		tctx.TC.Context(),
		0,
		func() error {
			r, err := call(tctx.TC.Context(), uri)
			if err != nil {
				var e net.Error
				if xerrors.As(err, &e) {
					return err
				}

				return backoff.Permanent(err)
			}

			tctx.RESTCtx.LastResponse = r
			return nil
		},
	)
}

func (tctx *testContext) weGETPillarConfig(path string) error {
	return tctx.doPillarConfig(
		path,
		func(ctx context.Context, uri string) (*resty.Response, error) {
			return tctx.RESTCtx.Client.R().SetContext(ctx).SetHeaders(tctx.RESTCtx.Headers).Get(uri)
		},
	)
}

func (tctx *testContext) weGetPillarConfigResponseBodyContains(body *gherkin.DocString) error {
	if !tctx.isLastAPI(APIPILLARCONFIG) {
		return nil
	}

	if tctx.RESTCtx.LastResponse.StatusCode() != http.StatusOK {
		return xerrors.Errorf(
			"response status code is %d but expected to be 200. Body: %s",
			tctx.RESTCtx.LastResponse.StatusCode(),
			tctx.RESTCtx.LastResponse.Body(),
		)
	}

	expected := make(map[string]interface{})
	if err := json.Unmarshal([]byte(body.Content), &expected); err != nil {
		return xerrors.Errorf("step input json unmarshal error for body %s: %w", body.Content, err)
	}

	actual := make(map[string]interface{})
	if err := json.Unmarshal(tctx.RESTCtx.LastResponse.Body(), &actual); err != nil {
		return xerrors.Errorf("response body json unmarshal error for body %s: %w", string(tctx.RESTCtx.LastResponse.Body()), err)
	}

	return diff.OptionalKeys(expected, actual)
}
