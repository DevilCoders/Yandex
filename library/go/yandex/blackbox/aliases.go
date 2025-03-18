package blackbox

import "fmt"

type UserAlias string

// Aliases description: https://doc.yandex-team.ru/Passport/AuthDevGuide/concepts/DB_About.html#DB_About__aliases
const (
	UserAliasPortal      UserAlias = "1"
	UserAliasMail        UserAlias = "2"
	UserAliasNarodmail   UserAlias = "3"
	UserAliasNarod       UserAlias = "4"
	UserAliasLite        UserAlias = "5"
	UserAliasSocial      UserAlias = "6"
	UserAliasPdd         UserAlias = "7"
	UserAliasPddalias    UserAlias = "8"
	UserAliasAltdomain   UserAlias = "9"
	UserAliasPhonish     UserAlias = "10"
	UserAliasPhonenumber UserAlias = "11"
	UserAliasMailish     UserAlias = "12"
	UserAliasYandexoid   UserAlias = "13"
	UserAliasKinopoiskid UserAlias = "15"
	UserAliasUberid      UserAlias = "16"
	UserAliasYambot      UserAlias = "17"
	UserAliasKolonkish   UserAlias = "18"
	UserAliasPublicID    UserAlias = "19"
	UserAliasOldPublicID UserAlias = "20"
	UserAliasNeophonish  UserAlias = "21"
)

func (u UserAlias) String() string {
	switch u {
	case UserAliasPortal:
		return "portal"
	case UserAliasMail:
		return "mail"
	case UserAliasNarodmail:
		return "narodmail"
	case UserAliasNarod:
		return "narod"
	case UserAliasLite:
		return "lite"
	case UserAliasSocial:
		return "social"
	case UserAliasPdd:
		return "pdd"
	case UserAliasPddalias:
		return "pddalias"
	case UserAliasAltdomain:
		return "altdomain"
	case UserAliasPhonish:
		return "phonish"
	case UserAliasPhonenumber:
		return "phonenumber"
	case UserAliasMailish:
		return "mailish"
	case UserAliasYandexoid:
		return "yandexoid"
	case UserAliasKinopoiskid:
		return "kinopoiskid"
	case UserAliasUberid:
		return "uberid"
	case UserAliasYambot:
		return "yambot"
	case UserAliasKolonkish:
		return "kolonkish"
	case UserAliasPublicID:
		return "publicid"
	case UserAliasOldPublicID:
		return "oldpublicid"
	case UserAliasNeophonish:
		return "neophonish"
	default:
		return fmt.Sprintf("unknown_%s", string(u))
	}
}
