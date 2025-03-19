Агент синхронизации Compute продуктов маркетплейса на разные стенды.

Short summary:
1. `internal/app/app.go` - инициализация сервиса, клиентов и чтение конфигов
2. `internal/services/sync/service.go` - листинг ямлов с данными о продуктах и версиях
3. `internal/core/actions/sync_product.go`, `internal/core/actions/sync_version.go` - основные actions, из которых через Adapter происходит поход в ручки MKT API

`ya make --add-result .go --replace-result` helps with proto generate
