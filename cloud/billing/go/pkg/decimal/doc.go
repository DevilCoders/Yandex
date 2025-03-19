/*
Package decimal provides tools for working with decimal values in Go code.

Decimal types are stored as int128 values and has constant precision of 38 and
scale of 15. This helps apply some optimizations for speed up arithmetics and
prevent most allocations during arithmetic actions.

Library has compatibility implementations for some standard libraries like:
- encoding/json
- database/sql
- fmt
- gopkg.in/yaml.v2
- etc

*/

package decimal
