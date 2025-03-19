package valid_test

import (
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/valid"
	libvalid "a.yandex-team.ru/library/go/valid"
)

func TestRegexp_Validate(t *testing.T) {
	vctx := libvalid.NewValidationCtx()

	proceed, err := (&valid.Regexp{Pattern: "[a-z]"}).Validate(vctx)
	assert.False(t, proceed)
	assert.NoError(t, err)

	proceed, err = (&valid.Regexp{Pattern: "\\"}).Validate(vctx)
	assert.False(t, proceed)
	assert.Error(t, err)
}

func TestRegexp_ValidateString(t *testing.T) {
	vctx := libvalid.NewValidationCtx()

	v := &valid.Regexp{Pattern: "[a-z]", Msg: "invalid %q"}
	_, err := v.Validate(vctx)
	require.NoError(t, err)

	assert.NoError(t, v.ValidateString("c"))
	assert.EqualError(t, v.ValidateString("1"), "invalid \"1\"")
}

func TestRegexp_ValidateStringNoPlaceholder(t *testing.T) {
	vctx := libvalid.NewValidationCtx()

	v := &valid.Regexp{
		Pattern: "[a-z]", Msg: "invalid",
	}
	_, err := v.Validate(vctx)
	require.NoError(t, err)

	assert.NoError(t, v.ValidateString("c"))
	assert.EqualError(t, v.ValidateString("1"), "invalid%!(EXTRA string=1)")
}

func TestRegexp_ValidateStringOmitMessageFormatting(t *testing.T) {
	vctx := libvalid.NewValidationCtx()

	v := &valid.Regexp{
		Pattern: "[a-z]", Msg: "invalid",
		InputErrorFormatter: valid.InputErrorFormatter{
			OmitMessageFormatting: true,
		},
	}
	_, err := v.Validate(vctx)
	require.NoError(t, err)

	assert.NoError(t, v.ValidateString("c"))
	assert.EqualError(t, v.ValidateString("1"), "invalid")
}

func TestStringLength_Validate(t *testing.T) {
	vctx := libvalid.NewValidationCtx()

	proceed, err := (&valid.StringLength{Min: 1, Max: 2}).Validate(vctx)
	assert.False(t, proceed)
	assert.NoError(t, err)

	proceed, err = (&valid.StringLength{Min: 1}).Validate(vctx)
	assert.False(t, proceed)
	assert.Error(t, err)
}

func TestStringLength_ValidateString(t *testing.T) {
	vctx := libvalid.NewValidationCtx()

	v := &valid.StringLength{Min: 1, Max: 3, TooShortMsg: "short %q", TooLongMsg: "long %q"}

	_, err := v.Validate(vctx)
	require.NoError(t, err)

	assert.NoError(t, v.ValidateString("a"))
	assert.NoError(t, v.ValidateString("ab"))
	assert.NoError(t, v.ValidateString("abc"))
	assert.EqualError(t, v.ValidateString(""), "short \"\"")
	assert.EqualError(t, v.ValidateString("abcd"), "long \"abcd\"")
}

func TestStringBlacklist_Validate(t *testing.T) {
	vctx := libvalid.NewValidationCtx()

	proceed, err := (&valid.StringBlacklist{}).Validate(vctx)
	assert.False(t, proceed)
	assert.NoError(t, err)

	proceed, err = (&valid.StringBlacklist{Blacklist: []string{"foo"}}).Validate(vctx)
	assert.False(t, proceed)
	assert.NoError(t, err)
}

func TestStringBlacklist_ValidateString(t *testing.T) {
	vctx := libvalid.NewValidationCtx()

	v := &valid.StringBlacklist{Blacklist: []string{"foo", "bar"}, Msg: "invalid %q"}
	_, err := v.Validate(vctx)
	require.NoError(t, err)

	assert.NoError(t, v.ValidateString("stuff"))
	assert.EqualError(t, v.ValidateString("foo"), "invalid \"foo\"")
	assert.EqualError(t, v.ValidateString("bar"), "invalid \"bar\"")
}

func TestPrefixBlacklist_Validate(t *testing.T) {
	vctx := libvalid.NewValidationCtx()

	proceed, err := (&valid.PrefixBlacklist{}).Validate(vctx)
	assert.False(t, proceed)
	assert.NoError(t, err)

	proceed, err = (&valid.PrefixBlacklist{Blacklist: []string{"foo"}}).Validate(vctx)
	assert.False(t, proceed)
	assert.NoError(t, err)
}

func TestPrefixBlacklist_ValidateString(t *testing.T) {
	vctx := libvalid.NewValidationCtx()

	v := &valid.PrefixBlacklist{Blacklist: []string{"foo_", "bar_"}, Msg: "invalid %q"}
	_, err := v.Validate(vctx)
	require.NoError(t, err)

	assert.NoError(t, v.ValidateString("stuff_foo_bar_"))
	assert.NoError(t, v.ValidateString("stuff"))
	assert.NoError(t, v.ValidateString(""))
	assert.EqualError(t, v.ValidateString("foo_stuff"), "invalid \"foo_stuff\"")
	assert.EqualError(t, v.ValidateString("bar_stuff"), "invalid \"bar_stuff\"")
}

func TestStringComposedValidator_Validate(t *testing.T) {
	v, err := valid.NewStringComposedValidator(&valid.StringBlacklist{Blacklist: []string{"foo"}}, &valid.StringLength{Min: 1, Max: 2})
	assert.NoError(t, err)
	assert.NotNil(t, v)

	v, err = valid.NewStringComposedValidator(&valid.StringBlacklist{Blacklist: []string{"foo"}}, &valid.StringLength{Min: 1})
	assert.Error(t, err)
	assert.Nil(t, v)
}

func TestStringComposedValidator_ValidateString(t *testing.T) {
	v, err := valid.NewStringComposedValidator(
		&valid.StringBlacklist{Blacklist: []string{"foo"}, Msg: "invalid %q"},
		&valid.StringLength{Min: 1, Max: 2, TooShortMsg: "short %q", TooLongMsg: "long %q"},
	)
	assert.NoError(t, err)
	assert.NotNil(t, v)

	assert.NoError(t, v.ValidateString("a"))
	assert.NoError(t, v.ValidateString("ab"))
	assert.EqualError(t, v.ValidateString("foo"), "invalid \"foo\"")
	assert.EqualError(t, v.ValidateString(""), "short \"\"")
	assert.EqualError(t, v.ValidateString("abc"), "long \"abc\"")
}
