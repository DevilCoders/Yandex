package resolving

import (
	"errors"
	"fmt"
	"io"
	"io/fs"

	"gopkg.in/yaml.v2"

	"a.yandex-team.ru/cloud/billing/go/pkg/selfsku/tools"
)

func LoadResolvingCases(files fs.FS) (result []TestCase, err error) {
	load := func(path string, r io.Reader) error {
		dec := yaml.NewDecoder(r)
		dec.SetStrict(true)
		cnt := 0
		for {
			val := TestCase{SourcePath: tools.SourcePath(fmt.Sprintf("%s/%d", path, cnt))}
			if err := dec.Decode(&val); err != nil {
				if errors.Is(err, io.EOF) {
					break
				}
				return err
			}
			result = append(result, val)
			cnt++
		}
		return nil
	}

	err = fs.WalkDir(files, "metrics", tools.LoadYAMLs(files, load))
	return
}
