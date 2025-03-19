package osmodels

import (
	"fmt"
	"net/http"
	"net/url"
	"strings"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
)

const MaxExtensionArhiveSize = 20 * 1024 * 1024 // 20Mb
const MaxExtensionsPerCluster = 20

type ExtensionSpec struct {
	Name     string
	URI      string
	Disabled bool
}

type Extension struct {
	ID      string `json:"id"`
	Name    string `json:"name"`
	Version int    `json:"version"`
	URI     string `json:"uri"`
	Active  bool   `json:"active"`
}

type Extensions struct {
	list []Extension
}

func NewExtensions(e []Extension) *Extensions {
	return &Extensions{e}
}

func (e *Extensions) Size() int {
	if e == nil {
		return 0
	}
	return len(e.list)
}

func (e *Extensions) GetList() []Extension {
	if e == nil {
		return nil
	}
	return e.list
}

func (e *Extensions) FindByID(id string) (Extension, error) {
	list := e.GetList()
	for i := range list {
		if list[i].ID == id {
			return list[i], nil
		}
	}
	return Extension{}, semerr.NotFoundf("extension with id %q is not found", id)
}

func (e *Extensions) Add(id, name, uri string, disabled bool) error {
	if len(e.list) >= MaxExtensionsPerCluster {
		return semerr.FailedPreconditionf("too many extensions")
	}

	if !disabled {
		e.deactivateAll(name)
	}

	e.list = append(e.list, Extension{
		ID:      id,
		Name:    name,
		Version: e.maxVersion(name) + 1,
		URI:     uri,
		Active:  !disabled,
	})

	return nil
}

func (e *Extensions) Delete(id string) error {
	list := e.GetList()
	for i := range list {
		if list[i].ID == id {
			e.list = append(list[:i], list[i+1:]...)
			return nil
		}
	}
	return semerr.NotFoundf("extension with id %q is not found", id)
}

func (e *Extensions) Update(id string, active bool) error {
	list := e.GetList()
	for i := range list {
		if list[i].ID == id {
			if active && !list[i].Active {
				e.deactivateAll(list[i].Name)
			}
			list[i].Active = active
			return nil
		}
	}
	return semerr.NotFoundf("extension with id %q is not found", id)
}

func (e *Extensions) deactivateAll(name string) {
	list := e.GetList()
	for i := range list {
		if list[i].Name == name && list[i].Active {
			list[i].Active = false
		}
	}
}

func (e *Extensions) maxVersion(name string) int {
	list := e.GetList()
	v := 0
	for i := range list {
		if list[i].Name == name && list[i].Version > v {
			v = list[i].Version
		}
	}
	return v
}

func ValidateExtensionURI(uri string, allowedDomain string) error {
	u, err := url.Parse(uri)
	if err != nil {
		return fmt.Errorf("can't parse uri")
	}

	if u.Scheme != "https" {
		return fmt.Errorf("invalid scheme, only https is supported")
	}

	if !strings.HasSuffix(u.Host, allowedDomain) {
		return fmt.Errorf("wrong host, should be in %q domain", allowedDomain)
	}

	return nil
}

// ValidateExtensionArchive pre-validates availability and size of provided archive
func ValidateExtensionArchive(uri string) error {
	res, err := http.Head(uri)
	if err != nil {
		return err
	}

	if res.StatusCode != 200 {
		return fmt.Errorf("uri is not available")
	}

	if res.ContentLength > MaxExtensionArhiveSize {
		return fmt.Errorf("archive is too large")
	}

	return nil
}
