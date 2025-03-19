package console

const (
	authTokenHeaderName = "X-YaCloud-SubjectToken"
)

type AuthData struct {
	Token string
}
