package ftpd

import (
	"encoding/json"
	"io"
	"io/ioutil"
	"regexp"

	"github.com/fclairamb/ftpserver/server"
	"golang.org/x/crypto/bcrypt"

	s3driver "a.yandex-team.ru/admins/ftp2s3/internal/s3-driver"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	ErrInvalidName         = xerrors.New("ftpd/auth: invalid name")
	ErrInvalidPassword     = xerrors.New("ftpd/auth: invalid password")
	ErrPasswordHashTooWeak = xerrors.Errorf("ftpd/auth: password hash too weak: %w", ErrInvalidPassword)
	ErrDuplicateUser       = xerrors.New("ftpd/auth: duplicate user")
	ErrInvalidLogin        = xerrors.New("ftpd/auth: login incorrect")
)

type User struct {
	Name     string `json:"name"`
	Password string `json:"password"`
}

type FtpdService struct {
	S3Service s3driver.S3API
	metrics   metrics.Registry

	Users map[string]string
}

func (service *FtpdService) getUsers(in io.Reader) error {
	service.Users = make(map[string]string)

	byteValue, err := ioutil.ReadAll(in)
	if err != nil {
		return err
	}

	var users []User
	err = json.Unmarshal(byteValue, &users)
	if err != nil {
		return err
	}
	re := regexp.MustCompile(`^[a-z0-9][a-z0-9\-]+$`)
	for _, user := range users {
		if !re.Match([]byte(user.Name)) {
			return xerrors.Errorf("User %q: %w", user.Name, ErrInvalidName)
		}
		cost, err := bcrypt.Cost([]byte(user.Password))
		if err != nil {
			return xerrors.Errorf("User %q: %v: %w", user.Name, err, ErrInvalidPassword)
		}
		if cost < bcrypt.DefaultCost {
			return xerrors.Errorf("User %q: (got %d, expected at least %d): %w", user.Name, cost, bcrypt.DefaultCost, ErrPasswordHashTooWeak)
		}
		if _, ok := service.Users[user.Name]; ok {
			return xerrors.Errorf("User %q: %w", user.Name, ErrDuplicateUser)
		}
		service.Users[user.Name] = user.Password
	}

	log.Infof("Passwd database OK, will serve %d users.", len(service.Users))

	return nil
}

func (service *FtpdService) AuthUser(cc server.ClientContext, username, password string) (server.ClientHandlingDriver, error) {
	service.metrics.Counter("user.login").Inc()

	if passwordHash, ok := service.Users[username]; ok {
		err := bcrypt.CompareHashAndPassword([]byte(passwordHash), []byte(password))
		if err == nil {
			return &ftpHandler{
				driver: &s3driver.S3Driver{
					Service: service.S3Service,
					Prefix:  "/" + username,
				},
				metrics: service.metrics.WithTags(map[string]string{"user": username}),
			}, nil
		}
		service.metrics.Counter("user.incorrectPassword").Inc()
		return nil, xerrors.Errorf("%v: %w", err, ErrInvalidPassword)
	}
	service.metrics.Counter("user.unknown").Inc()
	return nil, ErrInvalidLogin
}

func (service *FtpdService) WelcomeUser(cc server.ClientContext) (string, error) {
	cc.SetDebug(debug)
	return "Welcome!", nil
}

func (service *FtpdService) UserLeft(cc server.ClientContext) {
	service.metrics.Counter("user.logout").Inc()
}
