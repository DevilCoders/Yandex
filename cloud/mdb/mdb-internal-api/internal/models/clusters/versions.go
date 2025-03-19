package clusters

type Version struct {
	ID          string
	Name        string
	UpdatableTo []string
	Deprecated  bool
}
