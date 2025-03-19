package monitoring

type Monitoring struct {
	Charts []Chart
}

type Chart struct {
	Name        string
	Description string
	Link        string
}
