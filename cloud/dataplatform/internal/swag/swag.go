package swag

import (
	"bytes"
	"encoding/json"
	"html/template"
	"net/http"

	"github.com/go-openapi/spec"
	"github.com/grpc-ecosystem/grpc-gateway/runtime"

	"a.yandex-team.ru/cloud/dataplatform/internal/logger"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/resource"
)

const (
	redocTemplate = `<!-- HTML for static distribution bundle build -->
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8">
    <title>{{ .Title }}</title>
    <link rel="stylesheet" type="text/css" href="https://a.yandex-team.ru/api/swagger/swagger-ui.css" >
    <style>
      html
      {
        box-sizing: border-box;
        overflow: -moz-scrollbars-vertical;
        overflow-y: scroll;
      }
      *,
      *:before,
      *:after
      {
        box-sizing: inherit;
      }
      body
      {
        margin:0;
        background: #fafafa;
      }
    </style>
  </head>

  <body>
    <div id="swagger-ui"></div>
    <script src="https://a.yandex-team.ru/api/swagger/swagger-ui-bundle.js"></script>
    <script>
    window.onload = function() {
      // Begin Swagger UI call region
      const ui = SwaggerUIBundle({
        url: "{{ .SpecURL }}",
        dom_id: '#swagger-ui',
        deepLinking: true,
        presets: [
          SwaggerUIBundle.presets.apis,
        ],
        plugins: [
          SwaggerUIBundle.plugins.DownloadUrl
        ]
      })
      // End Swagger UI call region
      window.ui = ui
    }
  </script>
  </body>
</html>
`
)

type Opts struct {
	BasePath     string
	Path         string
	SpecURL      string
	RedocURL     string
	Title        string
	SpecPath     string
	SpecResource string
	ExternalDocs *spec.ExternalDocumentation
	Info         *spec.Info
}

type FuncOpts func(opts *Opts)

func WithInfo(info *spec.Info) FuncOpts {
	return func(opts *Opts) {
		opts.Info = info
	}
}

func WithExternalDocs(docs *spec.ExternalDocumentation) FuncOpts {
	return func(opts *Opts) {
		opts.ExternalDocs = docs
	}
}

func SpecResource(resource string, fOpts ...FuncOpts) Opts {
	var opts Opts
	opts.SpecResource = resource
	for _, fopt := range fOpts {
		fopt(&opts)
	}
	return opts
}

func Title(title string) Opts {
	var opts Opts
	opts.Title = title
	return opts
}

func (r *Opts) EnsureDefaults() {
	if r.SpecURL == "" {
		r.SpecURL = "/swagger.json"
	}
	if r.SpecPath == "" {
		r.SpecPath = "./swagger.json"
	}
	if r.SpecResource == "" {
		r.SpecResource = "spec.swagger.json"
	}
	if r.Title == "" {
		r.Title = "API documentation"
	}
}

func NewSwagUI(opts Opts, mux *runtime.ServeMux, others ...Opts) {
	opts.EnsureDefaults()
	tmpl := template.Must(template.New("redoc").Parse(redocTemplate))

	buf := bytes.NewBuffer(nil)
	_ = tmpl.Execute(buf, opts)
	b := buf.Bytes()

	mux.Handle(
		"GET",
		runtime.MustPattern(runtime.NewPattern(1, []int{2, 0}, []string{""}, "")),
		func(w http.ResponseWriter, r *http.Request, pathParams map[string]string) {

			w.Header().Set("Content-Type", "text/html; charset=utf-8")
			if _, err := w.Write(b); err != nil {
				logger.Log.Error("unable to write", log.Error(err))
				return
			}
		},
	)

	mux.Handle(
		"GET",
		runtime.MustPattern(runtime.NewPattern(1, []int{2, 0}, []string{"swagger.json"}, "")),
		func(w http.ResponseWriter, r *http.Request, pathParams map[string]string) {
			b := resource.Get(opts.SpecResource)
			mainSw := &spec.Swagger{}
			err := mainSw.UnmarshalJSON(b)
			if err != nil {
				logger.Log.Error("bad json", log.Error(err))
			}
			childSws := make([]*spec.Swagger, 0)
			for _, o := range others {
				b := resource.Get(o.SpecResource)
				childSw := &spec.Swagger{}
				err := childSw.UnmarshalJSON(b)
				if err != nil {
					logger.Log.Error("bad json", log.Error(err))
				} else {
					childSws = append(childSws, childSw)
				}
			}
			if len(childSws) > 0 {
				_ = Mixin(mainSw, childSws...)
			}
			if opts.ExternalDocs != nil {
				mainSw.ExternalDocs = opts.ExternalDocs
			}
			if opts.Info != nil {
				mainSw.Info = opts.Info
			}
			w.Header().Set("Content-Type", "Application/json")
			res, _ := json.Marshal(mainSw)
			if _, err := w.Write(res); err != nil {
				logger.Log.Error("unable to write", log.Error(err))
				return
			}
		},
	)
}
