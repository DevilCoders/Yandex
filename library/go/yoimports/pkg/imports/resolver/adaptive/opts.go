package adaptive

type Option func(*Resolver)

// WithDir allow to configure directory in which to run the build system's query tool that provides information about the packages.
// By default, the tool is run in the current directory.
func WithDir(dir string) Option {
	return func(r *Resolver) {
		r.dir = dir
	}
}

// WithFallbackAllowed enable or disable fallback to golist resolver.
func WithFallbackAllowed(allowed bool) Option {
	return func(r *Resolver) {
		r.fallbackAllowed = allowed
	}
}
