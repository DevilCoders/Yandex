package kafka

type TopicPageToken struct {
	LastTopicName string
	More          bool `json:"-"`
}

func (cpt TopicPageToken) HasMore() bool {
	return cpt.More
}
