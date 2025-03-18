package repos

type Repo struct {
	Name        string
	Description string
	URL         string
}

var Repos = map[string]*Repo{
	"hasql": {
		Name:        "hasql",
		Description: "Go library for high availability SQL installations with clustering",
		URL:         "https://github.com/yandex/go-hasql",
	},
}
