package a

import (
	"bytes"
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"io/ioutil"
	"math/rand"
	"os"
	"strconv"
	"time"
)

func functionReturnsError() error {
	return errors.New("errcheck: functionReturnsError")
}

func functionReturnsErrorChan() <-chan error {
	ch := make(chan error)
	return ch
}

var testSentinelError = errors.New("sentinel error")

var testDeclErrDrop, _ = strconv.ParseBool(os.Getenv("ALLOW_UNHANDLED_ERRORS"))

func noErrors() {
	i := rand.Intn(42)
	_ = i
}

func checkedError() {
	i := rand.Intn(42)
	_, err := fmt.Fprintf(os.Stdout, "the answer is %d", i)
	if err != nil {
		panic(err)
	}
}

func droppedError() {
	i := rand.Intn(42)
	_, _ = fmt.Fprintf(os.Stdout, "the answer is %d", i)
}

func deferedError() {
	buf := bytes.NewBufferString("ololo")
	defer ioutil.NopCloser(buf).Close()
}

func droppedReturnError() {
	_ = functionReturnsError()
}

func functionReturnsTupleWithError() (string, error) {
	return "ololo", errors.New("errcheck: functionReturnsError")
}

func droppedReturnTupleWithError() {
	msg, _ := functionReturnsTupleWithError()
	_ = msg
}

func functionMultireturnWithError() (string, int, float32, error) {
	return "ololo", 42, 14.89, errors.New("errcheck: functionReturnsError")
}

func checkedMultireturnWithError() {
	msg, id, price, err := functionMultireturnWithError()
	if err != nil {
		panic(err)
	}
	_, _, _ = msg, id, price
}

func checkedGoroutineError() {
	go func() { _ = functionReturnsError() }()
}

func must(v interface{}, _ error) interface{} {
	return v
}

func checkedWithMustError() {
	must(functionReturnsTupleWithError())
}

func checkedErrorInBinaryExpression() {
	ctx := context.Background()
	if ctx.Err() != nil {
		panic("ololo")
	}
}

func errorInSliceRange() {
	for _, e := range []error{
		testSentinelError,
		fmt.Errorf("error: %w", errors.New("test error")),
		errors.New("some error"),
	} {
		_ = e
	}
}

func errorInMapRange() {
	for k, e := range map[string]error{
		"field1": testSentinelError,
		"field2": fmt.Errorf("error: %w", errors.New("test error")),
		"field3": errors.New("some error"),
	} {
		_, _ = k, e
	}
}

//nolint:errcheck
func skippedGenDeclLint() {
	errors.New("error")
}

func skippedCallExprLint() {
	//nolint:errcheck
	errors.New("error")

	buf := bytes.NewBufferString("ololo")
	ioutil.NopCloser(buf).Close() // want `unhandled error`

	//nolint:errcheck
	_ = func() int {
		functionReturnsError()
		return 42
	}()
}

func errorSentToChannel() {
	ch := make(chan error)
	ch <- functionReturnsError()
}

func errorInSwitch() {
	switch functionReturnsError() {
	case testSentinelError:
		return
	default:
		panic("unknown error")
	}
}

func errorInSelect() {
	ticker := time.NewTicker(1 * time.Second)

	select {
	case <-ticker.C:
		fmt.Println("tick")
	case <-functionReturnsErrorChan():
		panic("error!!!")
	}
}

type o struct {
	e error
}

func errorInStructLiteral() {
	_ = o{
		e: errors.New("error"),
	}
}

func errorInSelector() {
	functionReturnsError().Error()
}

func FromError(err error) error {
	return nil
}

func convertError() {
	_ = json.NewEncoder(nil).Encode(FromError(errors.New("error")))
}

func uncheckedTupleError() {
	i := rand.Intn(42)
	fmt.Fprintf(os.Stdout, "the answer is %d", i)
}

func uncheckedSingleError() {
	buf := bytes.NewBufferString("ololo")
	ioutil.NopCloser(buf).Close() // want `unhandled error`
}

func uncheckedReturnError() {
	functionReturnsError() // want `unhandled error`
}

func uncheckedGoroutineError() {
	go functionReturnsError() // want `unhandled error`
}

func uncheckedReturnTupleWithError() {
	functionReturnsTupleWithError() // want `unhandled error`
}

func uncheckedMultireturnWithError() {
	functionMultireturnWithError() // want `unhandled error`
}

func uncheckedGoroutineErrorInsideFunc() {
	go func() {
		functionReturnsError() // want `unhandled error`
	}()
}

func uncheckedDeferErrorInsideFunc() {
	buf := bytes.NewBufferString("ololo")
	defer func() {
		ioutil.NopCloser(buf).Close() // want `unhandled error`
	}()
}

func doRange(f func()) {
}

func uncheckedSingleErrorInsideRange() {
	doRange(func() {
		ioutil.NopCloser(nil).Close() // want `unhandled error`
	})
}

func uncheckedErrorInsideReturn() {
	_ = func() error {
		return func() error {
			ioutil.NopCloser(nil).Close() // want `unhandled error`
			return nil
		}()
	}
}

func uncheckedErrorInsideGenDecl() {
	_ = func() error {
		ioutil.NopCloser(nil).Close() // want `unhandled error`
		return nil
	}()
}

func uncheckedErrorInsideVarDecl() {
	var err = func() error {
		ioutil.NopCloser(nil).Close() // want `unhandled error`
		return nil
	}()

	_ = err
}

func uncheckedErrorInsideErrorConsumer() {
	func(err error) {
		ioutil.NopCloser(nil).Close() // want `unhandled error`
	}(testSentinelError)
}

func uncheckedErrorInStructLiteral() {
	_ = o{
		e: func() error {
			errors.New("error") // want `unhandled error`
			return nil
		}(),
	}
}

func unhandledErrorInSwitch() {
	switch functionReturnsError() {
	case testSentinelError:
		return
	default:
		errors.New("error") // want `unhandled error`
	}
}
