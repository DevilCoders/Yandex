PY23_LIBRARY()

OWNER(mihajlova)

PY_SRCS(
    NAMESPACE nirvana_api.blocks
    base_block.py
    bash_command.py
    build_arcadia_project.py
    fml_pool_operations.py
    get_mr_table.py
    get_sandbox_resource.py
    gunzip_tsv.py
    matrixnet.py
    catboost.py
    processor_parameters.py
    file_format_converters.py
    svn_blocks.py
    __init__.py
)

END()
