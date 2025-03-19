package intervals

type BoundType int

const (
	Inclusive BoundType = iota + 1
	Exclusive
	Unbounded
)

var boundTypeMinToMathValue = map[BoundType]string{
	Inclusive: "[",
	Exclusive: "(",
	Unbounded: "(",
}

func (bt BoundType) MinString() string {
	s, ok := boundTypeMinToMathValue[bt]
	if !ok {
		return "?"
	}

	return s
}

var boundTypeMaxToMathValue = map[BoundType]string{
	Inclusive: "]",
	Exclusive: ")",
	Unbounded: ")",
}

func (bt BoundType) MaxString() string {
	s, ok := boundTypeMaxToMathValue[bt]
	if !ok {
		return "?"
	}

	return s
}
