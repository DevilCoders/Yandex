package bundler

import (
	"io"
	"io/fs"
	"path/filepath"
	"strings"

	"gopkg.in/yaml.v2"

	"a.yandex-team.ru/cloud/billing/go/pkg/selfsku/origin"
	"a.yandex-team.ru/cloud/billing/go/pkg/selfsku/tools"
)

func Load(files fs.FS) (result *Bundle, err error) {
	result = &Bundle{
		bundleSkus: make(map[string]origin.BundleSkus),
	}
	units, err := LoadUnits(files)
	if err != nil {
		return nil, err
	}
	for _, u := range units {
		result.units = append(result.units, u.UnitConversion)
	}
	services, err := LoadServices(files)
	if err != nil {
		return nil, err
	}
	for _, s := range services {
		result.services = append(result.services, s.Service)
	}
	skus, err := LoadSkus(files)
	if err != nil {
		return nil, err
	}

	for _, s := range skus {
		result.skus = append(result.skus, s.ServiceSkus)
	}

	bundles, err := FindBundles(files)
	if err != nil {
		return nil, err
	}
	for _, bundle := range bundles {
		skus, err := LoadBundle(files, bundle)
		if err != nil {
			return nil, err
		}
		for _, s := range skus {
			if _, ok := result.bundleSkus[bundle]; !ok {
				result.bundleSkus[bundle] = s.BundleSkus
				continue
			}
			for name, sku := range s.BundleSkus {
				result.bundleSkus[bundle][name] = sku
			}
		}
	}

	return result, err
}

func LoadUnits(files fs.FS) (result []UnitConversion, err error) {
	load := func(path string, r io.Reader) error {
		dec := yaml.NewDecoder(r)
		var val []origin.UnitConversion
		if err := dec.Decode(&val); err != nil {
			return err
		}
		for _, v := range val {
			result = append(result, UnitConversion{
				SourcePath:     sourcePath(path),
				UnitConversion: v,
			})
		}
		return nil
	}

	err = fs.WalkDir(files, "units", tools.LoadYAMLs(files, load))
	return
}

func LoadServices(files fs.FS) (result []Service, err error) {
	load := func(path string, r io.Reader) error {
		dec := yaml.NewDecoder(r)
		dec.SetStrict(true)
		val := Service{SourcePath: sourcePath(path)}
		if err := dec.Decode(&val.Service); err != nil {
			return err
		}
		result = append(result, val)
		return nil
	}

	err = fs.WalkDir(files, "services", tools.LoadYAMLs(files, load))
	return
}

func LoadSchemas(files fs.FS) (result []TagChecks, err error) {
	load := func(path string, r io.Reader) error {
		dec := yaml.NewDecoder(r)
		dec.SetStrict(true)
		val := TagChecks{SourcePath: sourcePath(path)}
		if err := dec.Decode(&val.TagChecks); err != nil {
			return err
		}
		result = append(result, val)
		return nil
	}

	err = fs.WalkDir(files, "schemas", tools.LoadYAMLs(files, load))
	return
}

func LoadSkus(files fs.FS) (result []ServiceSkus, err error) {
	load := func(path string, r io.Reader) error {
		dec := yaml.NewDecoder(r)
		dec.SetStrict(true)
		val := ServiceSkus{SourcePath: sourcePath(path)}
		if err := dec.Decode(&val.ServiceSkus); err != nil {
			return err
		}
		result = append(result, val)
		return nil
	}

	err = fs.WalkDir(files, "skus", tools.LoadYAMLs(files, load))
	return
}

func FindBundles(files fs.FS) (result []string, err error) {
	list := func(path string, d fs.DirEntry, err error) error {
		switch {
		case err != nil:
			return err
		case !d.IsDir():
			return nil
		case path == "bundles":
			return nil
		}

		name := d.Name()
		if !strings.HasPrefix(name, "_") && !strings.HasPrefix(name, ".") {
			result = append(result, name)
		}
		return fs.SkipDir
	}
	err = fs.WalkDir(files, "bundles", list)
	return
}

func LoadBundle(files fs.FS, name string) (skus []BundleSkus, err error) {
	load := func(path string, r io.Reader) error {
		dec := yaml.NewDecoder(r)
		dec.SetStrict(true)
		val := BundleSkus{SourcePath: sourcePath(path)}
		if err := dec.Decode(&val.BundleSkus); err != nil {
			return err
		}
		skus = append(skus, val)
		return nil
	}

	err = fs.WalkDir(files, filepath.Join("bundles", name), tools.LoadYAMLs(files, load))
	return
}
