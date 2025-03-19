# Build YC Search image

Profiles: prod, preprod
```
export PROFILE="prod"
export YAV_LOGIN=<domain_login> # if run on jenkins under ubuntu user
```

Create base image:
`make build-base`

Create images:
`make build-images`
