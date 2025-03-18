## valid

Библиотека валидации `valid` позволяет гибко описывать правила валидации различных значений и типов и производить саму
валидацию.

Библиотека вдохновлена пакетом `github.com/go-ozzo/ozzo-validation`

### Валидация единичного значения

Для валидации простой переменной достаточно задать правила валидации с помощью метода `Value` и вызвать метод `Validate`
.

Например:

```go
package main

import (
    "a.yandex-team.ru/library/go/valid/v2"
    "a.yandex-team.ru/library/go/valid/v2/rule"
)

func ExampleValidate() {
    creditCard := "4485321522253760"

    err := valid.Value(creditCard, rule.Len(16, 16), rule.Luhn).Validate()
    if err != nil {
        panic(err)
    }
}
```

Переменная может быть указателем или прямым значением.

Метод `Validate` всегда возвращает ошибку типа `Errors`, если есть хотя бы одна.

### Интерфейс Validator

Любой пользовательский тип может реализовать интерфейс `Validator`:

```
type Validator interface {
    Validate() error
}
```

Если тип реализует данный интерфейс, метод `Validate` будет вызван совместно с другими правилами валидации.

С помощью этого интерфейса удобно описывать правила валидции произвольных типов:

```go
package main

import (
    "time"

    "a.yandex-team.ru/library/go/valid/v2"
    "a.yandex-team.ru/library/go/valid/v2/rule"
)

type Account struct {
    Login      string
    Password   string
    LastSignIn time.Time
}

func (a *Account) Validate() error {
    return valid.Struct(a,
        valid.Value(&a.Login, rule.Required, rule.IsAlphanumeric),
        valid.Value(&a.Password, rule.Required, rule.IsAlphanumeric, rule.Len(6, -1)),
        valid.Value(&a.LastSignIn, rule.Required),
    )
}
```

### Валидация структуры

Для валидации структуры существует специальный метод `Struct`.

Пример:

```go
package main

import (
    "a.yandex-team.ru/library/go/valid/v2"
    "a.yandex-team.ru/library/go/valid/v2/rule"
)

func ExampleValidateStruct() {
    type User struct {
        ID         string
        Name       string
        Surname    string
        Patronymic string
        Age        int
        Job        string
    }

    u := User{
        ID:         "fefeefab-82f7-4b54-b3b6-1377b536ffa4",
        Name:       "Peter",
        Surname:    "Venkman",
        Patronymic: "",
        Age:        32,
        Job:        "Ghostbuster",
    }

    err := valid.Struct(&u,
        valid.Value(&u.ID, rule.Required, rule.IsUUID),            // check ID is present and is UUID
        valid.Value(&u.Name, rule.Required, rule.IsAlpha),         // check Name is present and consists only of ASCII letters
        valid.Value(&u.Surname, rule.Required, rule.IsAlpha),      // check Surname is present and consists only of ASCII letters
        valid.Value(&u.Patronymic, rule.OmitEmpty(rule.IsAlpha)),  // check Patronymic consists only of ASCII letters if not empty
        valid.Value(&u.Age, rule.OmitEmpty(rule.InRange(18, 99))), // check Age is between 18 and 99 years if not empty
        valid.Value(&u.Job, rule.OmitEmpty(rule.IsAlphanumeric)),  // check Job consists only of ASCII letters and digits if not empty
    )

    if err != nil {
        panic(err)
    }
}
```

Первым параметром всегда передается **указатель** на структуру, далее передаются правила валидации полей. Каждое поле
также должно передаваться в метод `Value` по указателю.

Метод `Struct` всегда возвращает ошибку типа `Errors`, если есть хотя бы одна. Однако в отличии от метода `Validate`
вложенные ошибки всегда будут иметь тип `FieldError`, содержащий ошибку и метаинформацию о поле структуры, в котором
произошла данная ошибка.

Если ошибка произошла во вложенном поле структуры - ошибка внутри `FieldError` также будет иметь тип `FieldError`.
Например:

```go
package main

import (
    "time"

    "a.yandex-team.ru/library/go/valid/v2"
    "a.yandex-team.ru/library/go/valid/v2/rule"
)

type User struct {
    Name       string
    Surname    string
    Patronymic string
    Account    Account
}

type Account struct {
    Login      string
    Password   string
    LastSignIn time.Time
}

// Структура Account реализует интерфейс Validator
func (a *Account) Validate() error {
    return valid.Struct(a,
        valid.Value(&a.Login, rule.Required, rule.IsAlphanumeric),
        valid.Value(&a.Password, rule.Required, rule.IsAlphanumeric, rule.Len(6, -1)),
        valid.Value(&a.LastSignIn, rule.Required),
    )
}

func ExampleNestedStruct() {
    u := User{
        Name:       "shimba",
        Surname:    "boomba",
        Patronymic: "",
        Account: Account{
            Login:      "looken",
            Password:   "123",
            LastSignIn: time.Now(),
        },
    }

    err := valid.Struct(&u,
        valid.Value(&u.Name, rule.Required, rule.IsAlpha),
        valid.Value(&u.Surname, rule.Required, rule.IsAlpha),
        valid.Value(&u.Patronymic, rule.OmitEmpty(rule.IsAlpha)),
        valid.Value(&u.Account),
    )

    if err != nil {
        panic(err)
    }
}
```

В данном примере ошибка в поле `User.Account.Password`, который должен быть длиннее 6 символов. Результирующая ошибка
будет иметь вид:

```
Errors{
    FieldError{
		Field: ..., // ссылка на поле Account в структуре User
		err: FieldError{
			Field: ..., // ссылка на поле Password в структуре Account
			err: rule.ErrInvalidStringLength,
        },
    },
}
```

Строковое представление такой ошибки будет выглядеть как:

```
Account.Password: invalid string length
```

### Произвольное сообщение об ошибке

Специальное правило `Message` позволяет обернуть полученную ошибку произвольным сообщением:

```go
package main

import (
    "a.yandex-team.ru/library/go/valid/v2"
    "a.yandex-team.ru/library/go/valid/v2/rule"
)

type User struct {
    Name       string
    Surname    string
    Patronymic string
}

func ExampleCustomMessage() {
    u := User{
        Name:       "sh1m_ba",
        Surname:    "boomba",
        Patronymic: "",
    }

    nameErrMessage := "имя обязательно и должно содержать только латинские буквы"

    err := valid.Struct(&u,
        valid.Value(&u.Name, rule.Message(nameErrMessage, rule.Required, rule.IsAlpha)),
        valid.Value(&u.Surname, rule.Required, rule.IsAlpha),
        valid.Value(&u.Patronymic, rule.OmitEmpty(rule.IsAlpha)),
    )

    if err != nil {
        panic(err)
    }
}
```
