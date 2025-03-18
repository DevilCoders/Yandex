package valid_test

import (
	"reflect"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
)

func noop(_ string) error { return nil }

func TestMergeContextsSimple(t *testing.T) {
	defaultVctx := valid.NewValidationCtx()

	myNewVctx := valid.NewValidationCtx()
	myNewVctx.Add("simple_validator", valid.WrapValidator(noop))

	defaultVctx.Merge(myNewVctx)

	_, ok := defaultVctx.Get("simple_validator")
	assert.True(t, ok)
}

var (
	ErrorA = xerrors.New("error A")
	ErrorB = xerrors.New("error B")
)

func generateErrorA(_ string) error { return ErrorA }
func generateErrorB(_ string) error { return ErrorB }

func TestMergeContextsReplace(t *testing.T) {
	defaultVctx := valid.NewValidationCtx()
	defaultVctx.Add("demo_validator", valid.WrapValidator(generateErrorA))

	myNewVctx := valid.NewValidationCtx()
	myNewVctx.Add("demo_validator", valid.WrapValidator(generateErrorB))

	defaultVctx.Merge(myNewVctx)

	errFunc, ok := defaultVctx.Get("demo_validator")
	assert.True(t, ok)
	assert.Equal(t, errFunc(reflect.ValueOf(""), ""), generateErrorB(""))
	assert.NotEqual(t, errFunc(reflect.ValueOf(""), ""), generateErrorA(""))
}
