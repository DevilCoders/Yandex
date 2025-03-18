package template

import (
	"html/template"
)

var GoGet = template.Must(template.New("go-get").Parse(contentGoGet))

const contentGoGet = `
<html lang="en">
<head>
    <meta http-equiv="refresh" content="5; url=https://pkg.go.dev/golang.yandex/{{ .Relpath }}">
    <meta name=go-import content="golang.yandex/{{ .Relpath }} git {{ .Repo.URL }}">
    <meta name="go-source"
          content="golang.yandex/{{ .Relpath }} {{ .Repo.URL }} {{ .Repo.URL }}/tree/master{/dir} {{ .Repo.URL }}/blob/master{/dir}/{file}#L{line}">
</head>
<body>
Redirecting to documentation...
</body>
</html>
`
