package gopackages

type Option func(*Resolver)

// WithDir allow to configure directory in which to run the build system's query tool that provides information about the packages.
// By default, the tool is run in the current directory.
func WithDir(dir string) Option {
	return func(r *Resolver) {
		r.dir = dir
	}
}
