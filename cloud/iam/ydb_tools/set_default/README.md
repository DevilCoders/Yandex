### YDB table column batch update utility

### Examples

##### Update column with a constant value

```shell
set_default \
  --endpoint <endpoint>:2135 \
  --database /global/iam \
  --column is_system False \
  /global/iam/hardware/default/identity/r3/roles
```

##### Update column with a value based on same column's value

```shell
set_default \
  --endpoint <endpoint>:2135 \
  --database /global/iam \
  --column login "'lol' + row['login']" \
  /global/iam/hardware/default/identity/r3/subjects/all
```

##### Update single column with a value based on other column's value

```shell
set_default \
  --endpoint <endpoint>:2135 \
  --database /global/iam \
  --fetch-all-columns \
  --column status "'BLOCKED' if 'leroy' in row['name'] else row['status']" \
  /global/iam/hardware/default/identity/r3/clouds
```
