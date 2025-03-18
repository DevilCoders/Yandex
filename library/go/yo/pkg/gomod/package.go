package gomod

type PackageError struct {
	ImportStack []string // shortest path from package named on command line to this one
	Pos         string   // position of error (if present, file:line:col)
	Err         string   // the error itself
}

type Package struct {
	ImportPath string
	Module     *Module

	Imports      []string
	TestImports  []string
	XTestImports []string

	IgnoredGoFiles []string

	// Error information
	Incomplete bool            // this package or a dependency has an error
	Error      *PackageError   // error loading package
	DepsErrors []*PackageError // errors loading dependencies
}
