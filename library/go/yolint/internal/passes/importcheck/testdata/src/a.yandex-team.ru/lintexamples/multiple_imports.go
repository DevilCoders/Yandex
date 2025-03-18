package lintexamples // want `use yoimports to reformat file imports`

import _ "errors"
import _ "fmt"                         // want `multiple imports must be merged into one`
import _ "log"                         // want `multiple imports must be merged into one`
import _ "os"                          // want `multiple imports must be merged into one`
import _ "vendored.package/gofrs/uuid" // want `multiple imports must be merged into one`
import _ "a.yandex-team.ru/lib"        // want `multiple imports must be merged into one`