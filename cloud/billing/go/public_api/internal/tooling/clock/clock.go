package clock

import "github.com/jonboulle/clockwork"

var (
	clock = clockwork.NewRealClock()
)

func SetFakeClock(fakeClock clockwork.FakeClock) {
	clock = fakeClock
}

func Get() clockwork.Clock {
	return clock
}
