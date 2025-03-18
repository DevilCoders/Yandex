package doc

import (
	"embed"
	"path/filepath"
	"strings"

	"a.yandex-team.ru/library/go/core/resource"
)

var specs = func() map[string]string {
	sp := make(map[string]string)

	for _, name := range resource.Keys() {
		if !strings.HasPrefix(name, "resfs/file/specs/") {
			continue
		}

		sp[filepath.Base(name)] = name
	}

	return sp
}()

//go:embed vendor/*
var vendor embed.FS
