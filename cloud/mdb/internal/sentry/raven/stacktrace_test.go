package raven

import (
	"errors"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestNewStackTraceEmpty(t *testing.T) {
	err := errors.New("foo")
	st := newStackTrace(err, 0, nil)
	require.NotNil(t, st)
	require.NotEmpty(t, st.Frames)
}

/*
func TestXErrorsFrame(t *testing.T) {
	inputs := []struct {
		Name string
		New  func() error
		Line int
	}{
		{
			Name: "xerrors.New",
			New: func() error {
				return xerrors.New("initial")
			},
			Line: 29,
		},
		{
			Name: "Wrapped xerrors",
			New: func() error {
				err := xerrors.New("initial")
				return xerrors.Errorf("wrapper: %w", err)
			},
			Line: 37,
		},
		{
			Name: "Semerr",
			New: func() error {
				return semerr.NotFound("not found")
			},
			Line: 44,
		},
		{
			Name: "Wrapped Semerr",
			New: func() error {
				err := errors.New("initial")
				return semerr.WrapWithNotFound(err, "not found")
			},
			Line: 52,
		},
	}

	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			err := input.New()
			fr, ok := xerrorsFrame(err)
			require.True(t, ok)

			const f = "a.yandex-team.ru/cloud/mdb/internal/sentry/raven.TestXErrorsFrame"
			require.Equal(t, f, fr.Function[:len(f)]) // Remove anonymous function suffix
			require.Equal(t, input.Line, fr.Line)
		})
	}
}

func TestXErrorsFrames(t *testing.T) {
	inputs := []struct {
		Name  string
		New   func() error
		Lines []int
	}{
		{
			Name: "xerrors.New",
			New: func() error {
				return xerrors.New("initial")
			},
			Lines: []int{80},
		},
		{
			Name: "Wrapped xerrors",
			New: func() error {
				err := xerrors.New("initial")
				return xerrors.Errorf("wrapper: %w", err)
			},
			Lines: []int{88, 87},
		},
		{
			Name: "Semantic Error",
			New: func() error {
				return semerr.NotFound("not found")
			},
			Lines: []int{95},
		},
		{
			Name: "Wrapped Semantic Error",
			New: func() error {
				err := semerr.NotFound("not found")
				return xerrors.Errorf("wrapper: %w", err)
			},
			Lines: []int{103, 102},
		},
		{
			Name: "Wrapping Semantic Error",
			New: func() error {
				err := xerrors.New("initial")
				return semerr.WrapWithNotFound(err, "not found")
			},
			Lines: []int{111, 110},
		},
		{
			Name: "Multiwrap with Semantic Error",
			New: func() error {
				err := xerrors.New("initial")
				err = semerr.WrapWithNotFound(err, "not found")
				return xerrors.Errorf("wrapper: %w", err)
			},
			Lines: []int{120, 119, 118},
		},
	}

	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			err := input.New()
			frames := xerrorsFrames(err)
			require.Len(t, frames, len(input.Lines))

			const f = "a.yandex-team.ru/cloud/mdb/internal/sentry/raven.TestXErrorsFrames"
			for i := range frames {
				require.Equal(t, f, frames[i].Function[:len(f)])
				require.Equal(t, input.Lines[i], frames[i].Line)
			}
		})
	}
}*/

func TestNewStackTraceFrames(t *testing.T) {
	defer xerrors.DefaultStackTraceMode()
	xerrors.EnableFrames()

	err := xerrors.New("new")
	err = xerrors.Errorf("errorf: %w", err)

	st := newStackTrace(err, 0, nil)
	require.Len(t, st.Frames, 4)

	require.Equal(t, "testing", st.Frames[0].Module)
	// We ignore rest of this frame since we do not want to depend on stdlib source

	require.Equal(t, "a.yandex-team.ru/cloud/mdb/internal/sentry/raven", st.Frames[1].Module)
	require.Equal(t, "TestNewStackTraceFrames", st.Frames[1].Function)
	require.Equal(t, 148, st.Frames[1].Lineno)

	require.Equal(t, "a.yandex-team.ru/cloud/mdb/internal/sentry/raven", st.Frames[2].Module)
	require.Equal(t, "TestNewStackTraceFrames", st.Frames[2].Function)
	require.Equal(t, 146, st.Frames[2].Lineno)

	require.Equal(t, "a.yandex-team.ru/cloud/mdb/internal/sentry/raven", st.Frames[3].Module)
	require.Equal(t, "TestNewStackTraceFrames", st.Frames[3].Function)
	require.Equal(t, 145, st.Frames[3].Lineno)
}

func TestNewStackTraceStacks(t *testing.T) {
	defer xerrors.DefaultStackTraceMode()
	xerrors.EnableStacks()

	err := xerrors.New("new")
	err = xerrors.Errorf("errorf: %w", err)

	st := newStackTrace(err, 0, nil)
	require.Len(t, st.Frames, 2)

	require.Equal(t, "testing", st.Frames[0].Module)
	// We ignore rest of this frame since we do not want to depend on stdlib source

	require.Equal(t, "a.yandex-team.ru/cloud/mdb/internal/sentry/raven", st.Frames[1].Module)
	require.Equal(t, "TestNewStackTraceStacks", st.Frames[1].Function)
	require.Equal(t, 171, st.Frames[1].Lineno)
}
