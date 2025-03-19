package features

type Setter interface {
	set(*Flags)
}

func (f *Flags) Set(setters ...Setter) {
	for _, s := range setters {
		s.set(f)
	}
}

func SetDefault(f Flags) {
	defaultFlags = f
}

func Default() Flags {
	return defaultFlags
}
