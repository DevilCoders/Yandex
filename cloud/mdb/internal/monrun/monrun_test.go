package monrun_test

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/monrun"
)

func TestResultCodeConvs(t *testing.T) {
	tests := []struct {
		code    monrun.ResultCode
		codeStr string
	}{
		{monrun.OK, "OK"},
		{monrun.WARN, "WARN"},
		{monrun.CRIT, "CRIT"},
	}
	for _, tt := range tests {
		t.Run("ResultCodeFromString for"+tt.codeStr, func(t *testing.T) {
			ret, err := monrun.ResultCodeFromString(tt.codeStr)
			require.Equal(t, tt.code, ret)
			require.NoError(t, err)
		})
		t.Run(tt.codeStr+".String()", func(t *testing.T) {
			require.Equal(t, tt.codeStr, tt.code.String())
		})
	}
	t.Run("Bad code from string", func(t *testing.T) {
		_, err := monrun.ResultCodeFromString("FOO")
		require.Error(t, err)
	})
	t.Run("Bad code to sgring", func(t *testing.T) {
		require.Panics(t, func() {
			badCode := monrun.ResultCode(42)
			_ = badCode.String()
		})
	})
}

func TestEmptyResultIsOK(t *testing.T) {
	require.Equal(t, "0;OK", monrun.Result{}.String())
}

func TestCodes(t *testing.T) {
	t.Run("WARN", func(t *testing.T) {
		require.Equal(
			t,
			"1;warning message text",
			monrun.Result{
				Code:    monrun.WARN,
				Message: "warning message text",
			}.String(),
		)
	})
	t.Run("CRIT", func(t *testing.T) {
		require.Equal(
			t,
			"2;critical message text",
			monrun.Result{
				Code:    monrun.CRIT,
				Message: "critical message text",
			}.String())
	})
}

func TestSanitizeString(t *testing.T) {
	tests := []struct {
		name  string
		check monrun.Result
		want  string
	}{
		{
			"';' is removed",
			monrun.Result{
				Code:    monrun.WARN,
				Message: "foo;bar",
			},
			"1;foo bar",
		},
		{
			"endline removed",
			monrun.Result{
				Code:    monrun.WARN,
				Message: "foo\nbar",
			},
			"1;foo bar",
		},
		{
			"endlines and ';' removed",
			monrun.Result{
				Code:    monrun.WARN,
				Message: "WAT\nMAN;\n!!!",
			},
			"1;WAT MAN !!!",
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			require.Equal(t, tt.want, tt.check.String())
		})
	}
}

func TestWarnf(t *testing.T) {
	require.Equal(
		t, monrun.Result{
			Code:    monrun.WARN,
			Message: "foo: bar",
		}, monrun.Warnf("foo: %s", "bar"),
	)
}

func TestCritf(t *testing.T) {
	require.Equal(
		t, monrun.Result{
			Code:    monrun.CRIT,
			Message: "foo: bar",
		}, monrun.Critf("foo: %s", "bar"),
	)
}
