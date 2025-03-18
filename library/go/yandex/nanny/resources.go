package nanny

import "a.yandex-team.ru/library/go/x/encoding/unknownjson"

type Resources struct {
	SandboxFiles []SandboxFile `json:"sandbox_files"`
	StaticFiles  []StaticFile  `json:"static_files"`

	Unknown unknownjson.Store `json:"-" unknown:",store"`
}

func (r *Resources) UpdateStaticFile(path string, content string) {
	for i, f := range r.StaticFiles {
		if f.LocalPath == path {
			r.StaticFiles[i].Content = content
			return
		}
	}

	r.StaticFiles = append(r.StaticFiles, StaticFile{
		LocalPath: path,
		Content:   content,
	})
}
