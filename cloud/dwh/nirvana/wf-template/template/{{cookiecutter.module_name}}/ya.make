OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    {{cookiecutter.resources_path_from_root}}/query.sql {{ cookiecutter.resources_path_from_workflows }}/query.sql
    {{cookiecutter.resources_path_from_root}}/parameters.yaml {{ cookiecutter.resources_path_from_workflows }}/parameters.yaml
)

END()
