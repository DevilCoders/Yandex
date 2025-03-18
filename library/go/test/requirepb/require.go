package requirepb

import "a.yandex-team.ru/library/go/test/assertpb"

func Equal(t assertpb.TestingT, expected, actual interface{}, msgAndArgs ...interface{}) {
	t.Helper()

	if !assertpb.Equal(t, expected, actual, msgAndArgs...) {
		t.FailNow()
	}
}

func Equalf(t assertpb.TestingT, expected, actual interface{}, msg string, args ...interface{}) {
	t.Helper()

	Equal(t, expected, actual, append([]interface{}{msg}, args...)...)
}
