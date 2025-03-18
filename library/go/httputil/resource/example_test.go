package resource_test

import (
	"net/http"

	"a.yandex-team.ru/library/go/httputil/resource"
)

func Example_stdlib() {
	uriPath := "/static/"
	http.Handle(uriPath, http.StripPrefix(uriPath, http.FileServer(resource.Dir("/static/"))))
}
