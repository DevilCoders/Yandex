## Code style
1. [Here](https://docs.yandex-team.ru/arcadia-python/python_style_guide) you can read about python code style
2. Use flake8 as a linter ([flake8 config](https://a.yandex-team.ru/svn/trunk/arcadia/build/config/tests/flake8/flake8.conf) also can be found in the project root folder) and black as a code formatter [black config](https://a.yandex-team.ru/svn/trunk/arcadia/devtools/ya/handlers/style/python_style_config.toml)
3. How to use it?
#### CLI
- flake8 official [documentation](https://flake8.pycqa.org/en/latest/)
- black official [documentation](https://black.readthedocs.io/en/stable/usage_and_configuration/the_basics.html)
- ya style (already configured black formatter) official [documentation](https://docs.yandex-team.ru/yatool/commands/style)
#### PyCharm
- flake8 [guide](https://melevir.medium.com/pycharm-loves-flake-671c7fac4f52)
- black [guide](https://black.readthedocs.io/en/stable/integrations/editors.html#pycharm-intellij-idea)
#### NeoVim
- flake8 [guide](https://github.com/python-lsp/python-lsp-server#3rd-party-plugins)
- black (I think the simpliest way is to create keymap that calls black from [CLI](https://black.readthedocs.io/en/stable/usage_and_configuration/the_basics.html#command-line-options))
