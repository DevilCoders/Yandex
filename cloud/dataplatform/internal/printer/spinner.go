package printer

type Indicator struct {
}

func (s *Indicator) Stop() {
}

func Spin(label string) *Indicator {
	return &Indicator{}
}
