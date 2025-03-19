package tests

import (
	"encoding/json"
	"fmt"
	"strings"
	"time"

	"github.com/DATA-DOG/godog/gherkin"
	"github.com/golang/protobuf/jsonpb"
	"github.com/google/go-cmp/cmp"

	mlock "a.yandex-team.ru/cloud/mdb/mlock/api"
)

func (tctx *TestContext) waitReady() error {
	const tries = 30
	var err error

	for i := 0; i < tries; i++ {
		_, err := tctx.Client.ListLocks(
			tctx.UtilContext.Context(),
			&mlock.ListLocksRequest{},
		)
		if err != nil {
			time.Sleep(1 * time.Second)
			continue
		}
		return nil
	}

	return err
}

func (tctx *TestContext) releaseLock(id string) error {
	resp, err := tctx.Client.ReleaseLock(
		tctx.UtilContext.Context(),
		&mlock.ReleaseLockRequest{Id: id},
	)

	tctx.LastError = err
	tctx.LastResponse = resp

	return nil
}

func (tctx *TestContext) getLockStatus(id string) error {
	resp, err := tctx.Client.GetLockStatus(
		tctx.UtilContext.Context(),
		&mlock.GetLockStatusRequest{Id: id},
	)

	tctx.LastError = err
	tctx.LastResponse = resp

	return nil
}

func (tctx *TestContext) checkStatus(data *gherkin.DocString) error {
	if tctx.LastResponse == nil {
		return fmt.Errorf("expected response got nil with error: %w", tctx.LastError)
	}

	var actual map[string]interface{}

	marshaler := jsonpb.Marshaler{EmitDefaults: true, OrigName: true}

	jsonString, err := marshaler.MarshalToString(tctx.LastResponse)
	if err != nil {
		return fmt.Errorf("unable to marshal response %s into string: %w", tctx.LastResponse.String(), err)
	}

	if err = json.Unmarshal([]byte(jsonString), &actual); err != nil {
		return fmt.Errorf("unable to unmarshal response %s into json: %w", jsonString, err)
	}

	delete(actual, "create_ts")

	var expected map[string]interface{}

	if err = json.Unmarshal([]byte(data.Content), &expected); err != nil {
		return fmt.Errorf("unable to unmarshal expected value %s: %w", data.Content, err)
	}

	diff := cmp.Diff(expected, actual)

	if diff != "" {
		return fmt.Errorf("expected %q, actual %q, diff: %s", expected, actual, diff)
	}

	return nil
}

func (tctx *TestContext) checkList(data *gherkin.DocString) error {
	if tctx.LastResponse == nil {
		return fmt.Errorf("expected response got nil with error: %w", tctx.LastError)
	}

	var actual map[string]interface{}

	marshaler := jsonpb.Marshaler{EmitDefaults: true, OrigName: true}

	jsonString, err := marshaler.MarshalToString(tctx.LastResponse)
	if err != nil {
		return fmt.Errorf("unable to marshal response %s into string: %w", tctx.LastResponse.String(), err)
	}

	if err = json.Unmarshal([]byte(jsonString), &actual); err != nil {
		return fmt.Errorf("unable to unmarshal response %s into json: %w", jsonString, err)
	}

	locks, ok := actual["locks"]
	if ok {
		typedLocks, ok := locks.([]interface{})
		if ok {
			for _, lock := range typedLocks {
				typedLock, ok := lock.(map[string]interface{})
				if ok {
					delete(typedLock, "create_ts")
				}
			}
		}
	}

	var expected map[string]interface{}

	if err = json.Unmarshal([]byte(data.Content), &expected); err != nil {
		return fmt.Errorf("unable to unmarshal expected value %s: %w", data.Content, err)
	}

	diff := cmp.Diff(expected, actual)

	if diff != "" {
		return fmt.Errorf("expected %q, actual %q, diff: %s", expected, actual, diff)
	}

	return nil
}

func (tctx *TestContext) checkError(pattern string) error {
	if tctx.LastError == nil {
		return fmt.Errorf("expected error but got nil with response: %s", tctx.LastResponse.String())
	}

	if strings.Contains(tctx.LastError.Error(), pattern) {
		return nil
	}

	return fmt.Errorf("unable to find %s in %w", pattern, tctx.LastError)
}

func (tctx *TestContext) noError() error {
	if tctx.LastError != nil {
		return fmt.Errorf("expected nil but got error: %w", tctx.LastError)
	}

	return nil
}

func (tctx *TestContext) createLock(data *gherkin.DocString) error {
	request := mlock.CreateLockRequest{}

	if err := jsonpb.UnmarshalString(data.Content, &request); err != nil {
		return fmt.Errorf("unable to marshal data %s into protobuf: %w", data.Content, err)
	}

	resp, err := tctx.Client.CreateLock(
		tctx.UtilContext.Context(),
		&request,
	)

	tctx.LastError = err
	tctx.LastResponse = resp

	return nil
}

func (tctx *TestContext) listLocks(data *gherkin.DocString) error {
	request := mlock.ListLocksRequest{}

	if err := jsonpb.UnmarshalString(data.Content, &request); err != nil {
		return fmt.Errorf("unable to marshal data %s into protobuf: %w", data.Content, err)
	}

	resp, err := tctx.Client.ListLocks(
		tctx.UtilContext.Context(),
		&request,
	)

	tctx.LastError = err
	tctx.LastResponse = resp

	return nil
}
