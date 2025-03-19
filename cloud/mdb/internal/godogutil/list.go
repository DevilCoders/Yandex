package godogutil

import (
	"bytes"
	"fmt"
	"io"
	"os"
	"path/filepath"
	"strings"

	"github.com/DATA-DOG/godog/gherkin"
)

// Scenario is Ghrekin Scenario or Scenario Outline
type ScenarioDefinition struct {
	Name   string
	LineNo int
}

// Feature is Ghrekin feature
type Feature struct {
	Path      string
	Name      string
	Scenarios []ScenarioDefinition
}

func newFeature(feature *gherkin.Feature, path string) (f Feature) {
	f.Name = feature.Name
	f.Path = path
	for _, s := range feature.ScenarioDefinitions {
		switch s := s.(type) {
		case *gherkin.Scenario:
			f.Scenarios = append(f.Scenarios, ScenarioDefinition{Name: s.Name, LineNo: s.Node.Location.Line})
		case *gherkin.ScenarioOutline:
			f.Scenarios = append(f.Scenarios, ScenarioDefinition{Name: s.Name, LineNo: s.Node.Location.Line})
		}
	}
	return
}

func hasTag(tags []*gherkin.Tag, tag string) bool {
	for _, t := range tags {
		if t.Name == tag {
			return true
		}
	}
	return false
}

// based on http://behat.readthedocs.org/en/v2.5/guides/6.cli.html#gherkin-filters
func matchesTags(filter string, tags []*gherkin.Tag) (ok bool) {
	ok = true
	for _, andTags := range strings.Split(filter, "&&") {
		var okComma bool
		for _, tag := range strings.Split(andTags, ",") {
			tag = strings.TrimSpace(tag)
			if tag[0] == '~' {
				tag = tag[1:]
				okComma = !hasTag(tags, tag) || okComma
			} else {
				okComma = hasTag(tags, tag) || okComma
			}
		}
		ok = ok && okComma
	}
	return
}

// ListFeatures return features in given paths
func ListFeatures(paths []string, tags string) ([]Feature, error) {
	features := make(map[string]Feature)
	for _, pat := range paths {
		err := filepath.Walk(pat, func(p string, f os.FileInfo, err error) error {
			if err == nil && !f.IsDir() && strings.HasSuffix(p, ".feature") {
				reader, err := os.Open(p)
				if err != nil {
					return err
				}
				var buf bytes.Buffer
				feature, err := gherkin.ParseFeature(io.TeeReader(reader, &buf))
				_ = reader.Close()
				if err != nil {
					return fmt.Errorf("%s - %v", p, err)
				}

				if len(tags) == 0 || matchesTags(tags, feature.Tags) {
					features[p] = newFeature(feature, p)
				}
			}
			return err
		})

		switch {
		case os.IsNotExist(err):
			return nil, fmt.Errorf(`feature path "%s" is not available`, pat)
		case os.IsPermission(err):
			return nil, fmt.Errorf(`feature path "%s" is not accessible`, pat)
		case err != nil:
			return nil, err
		}
	}

	var res []Feature
	for _, f := range features {
		res = append(res, f)
	}
	return res, nil
}

// ListFeaturesMust return features in given paths. Panic if errors happens
func ListFeaturesMust(paths []string) []Feature {
	ret, err := ListFeatures(paths, "")
	if err != nil {
		panic(err)
	}
	return ret
}

// ListTests for arcadia -test.list run
func ListTests(paths []string, tags string) {
	for _, arg := range os.Args[1:] {
		if arg == "-test.list" {
			features, err := ListFeatures(paths, tags)
			if err != nil {
				fmt.Printf("Failed to list features: %s\n", err)
				os.Exit(1)
			}

			for _, f := range features {
				fmt.Println(formatTestName(f.Name))
			}

			os.Exit(0)
		}
	}
}
