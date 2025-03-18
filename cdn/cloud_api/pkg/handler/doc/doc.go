package doc

import (
	"fmt"
	"html/template"
	"net/http"
	"strings"

	"github.com/go-chi/chi/v5"

	"a.yandex-team.ru/library/go/core/resource"
)

const index = `<!DOCTYPE html>
<html>
  <head>
    <title>{{ . }} - Redoc</title>
    <!-- needed for adaptive design -->
    <meta charset="utf-8"/>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link href="https://fonts.googleapis.com/css?family=Montserrat:300,400,700|Roboto:300,400,700" rel="stylesheet">

    <!--
    Redoc doesn't change outer page styles
    -->
    <style>
      body {
        margin: 0;
        padding: 0;
      }
    </style>
  </head>
  <body>
    <redoc spec-url="specs/{{ . }}"></redoc>
    <script src="vendor/redoc.standalone.js"></script>
  </body>
</html>
`

var indexTemplate = template.Must(template.New("index").Parse(index))

func NewDocHandler() http.Handler {
	mux := chi.NewMux()

	mux.Get("/", indexHandler)
	mux.Mount("/specs/{spec}", specHandler())
	mux.Mount("/vendor", embedHandler(http.FS(vendor)))

	return mux
}

func indexHandler(w http.ResponseWriter, r *http.Request) {
	if r.URL.Path[len(r.URL.Path)-1] != '/' {
		http.Redirect(w, r, r.URL.Path+"/", http.StatusTemporaryRedirect)
		return
	}

	spec := r.URL.Query().Get("spec")
	if spec == "" {
		listSpecs(w, r)
		return
	}

	if err := indexTemplate.Execute(w, spec); err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
}

func listSpecs(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "text/html; charset=utf-8")

	_, _ = fmt.Fprint(w, "<pre>\nAvailable specs:\n")
	for specName := range specs {
		specURL := r.URL
		query := specURL.Query()
		query.Set("spec", specName)
		specURL.RawQuery = query.Encode()

		_, _ = fmt.Fprintf(w, "<a href=\"%s\">%s</a>\n", specURL.String(), specName)
	}
	_, _ = fmt.Fprint(w, "</pre>\n")
}

func embedHandler(root http.FileSystem) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		rctx := chi.RouteContext(r.Context())

		prefix := strings.TrimSuffix(rctx.RoutePattern(), "/*")
		if idx := strings.LastIndex(prefix, "/"); idx > 0 {
			prefix = prefix[:idx]
		}

		http.StripPrefix(prefix, http.FileServer(root)).ServeHTTP(w, r)
	}
}

func specHandler() http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		specName := chi.URLParam(r, "spec")
		spec := resource.Get(specs[specName])
		if spec == nil {
			http.NotFound(w, r)
			return
		}

		w.Header().Set("Content-Type", "application/json; charset=utf-8")
		_, _ = w.Write(spec)
	}
}
