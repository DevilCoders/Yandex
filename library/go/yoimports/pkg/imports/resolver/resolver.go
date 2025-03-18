package resolver

type Resolver interface {
	ResolvePackages(paths ...string) ([]Package, error)
}

type Package struct {
	Name string
}
