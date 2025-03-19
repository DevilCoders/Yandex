package tools

type SourcePath string

func (p SourcePath) LoadedFrom() string {
	return string(p)
}
