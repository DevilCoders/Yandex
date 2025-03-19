## Environment Configs

This folder contains environment-specific overrides for iam metadata.

Commands from group `ycp iam inner metadata` will pick up a file either explicitly specified (--env-file flag) or auto discovered from a ycp profile.
Auto discovery is simply checking for a file named `%profile%.yaml` in `private-api/yandex/cloud/priv/env/`.


Currently, there are two kinds of overrides supported in env files - role visibility and OAuth client settings.

### Role visibility

Roles should not become visible in public immediately, they need to be tested in testing/preprod first.
Also, there are other installations that may require a completely different set of publicly available roles. 

#### Via explicit override

Role visibility may explicitly be overridden like this:

```yaml
roles:
  alb.user: { visibility: internal }
```

This approach is used on non-production environments, so new roles are created as public without additional configuration.
But still, we have a lot (~70 at the moment) of public roles that were not actually enabled in any environment.
Visibility of these roles is explicitly set to internal just to maintain the status-quo.

#### Via allowed list

The other approach is to define enumerate roles that are allowed to be public.
If such a list is defined for an environment, then only those roles are public that both
* have `visibility: public` attribute,
* included in `allowed_public_roles`

```yaml
allowed_public_roles:
  - admin
  - viewer
  - editor
  - alb.editor
  - alb.viewer
```

We utilise this approach in `prod.yaml` because it's the most important to not publish roles in production unintentionally.

### OAuth client settings

OAuth clients have some attributes that are naturally distinct on different environments - secret and redirect urls.
Besides, some clients only need to exist in certain environments.

Here's an example of how client definition is usually split between files.

```yaml
# in clients.yaml, common for all environments
oauth_clients:
  yc.oauth.datalens:
    name: 'Datalens WEB-application'
    includedRoles:
      - billing.clouds.owner
      - datalens.instances.admin
    scopes:
      - yc.resource-manager.{clouds,folders}.get
      - yc.iam.serviceAccounts.{get,manage,use}
```

```yaml
# in env files - prod.yaml, testing.yaml, etc
oauth_clients:
  yc.oauth.datalens:
    authorized_grant_types: [ authorization_code ]
    client_scopes: [ openid ]
    auto_approve_scopes: [ openid ]
    client_secret_sha256: 34e4a5af4096c45a79bde0acbee7abdbb48b299ca5029dde372fe2eff345fac7
    redirect_uris:
      - https://datalens-preprod.yandex.ru/auth/callback
      # .. other redirect urls
``` 

If a client defined in `clients.yaml` and not in an env file, it will not be created on that environment.
(impl note: now we just check if client_secret_sha256 is defined)

If an env-specific client is openid-only, meaning, for authentication without authorization,
then it's ok to define it only in the env file.
