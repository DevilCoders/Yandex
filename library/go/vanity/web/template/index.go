package template

import (
	"html/template"
)

var Index = template.Must(template.New("index").Parse(contentIndex))

const contentIndex = `
<html lang="en">
<head>
    <meta charset="utf-8">
    <title>Yandex Open Source Go Libraries</title>
    <link rel="stylesheet" href="//fonts.googleapis.com/css?family=Open+Sans:300,300italic,700,700italic">
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/normalize/8.0.1/normalize.min.css">
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/milligram/1.3.0/milligram.min.css">
</head>
<body>
<section class="container">
    <table>
        <thead>
        <tr>
            <th>Library</th>
            <th>Description</th>
            <th>Source Repository</th>
        </tr>
        </thead>
        <tbody>
        {{ range $relpath, $repo := . }}
        <tr>
            <td>{{ $repo.Name }}</td>
            <td>{{ $repo.Description }}</td>
            <td><a href="{{ $repo.URL }}">{{ $repo.URL }}</a></td>
        </tr>
        {{ end }}
        </tbody>
    </table>
</section>
</body>
</html>
`
