package tooling

type IntObserver interface {
	ObserveInt(int)
}

type floatObserver interface {
	Observe(float64)
}

type observerWrapper struct {
	floatObserver
}

func (w observerWrapper) ObserveInt(v int) {
	w.floatObserver.Observe(float64(v))
}
