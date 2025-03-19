# **Contributing**

You may add fixes or features to current library.
You have to do it in a separate branch in [arc](https://docs.yandex-team.ru/arc/). After your code is completed you may create pull-request on merging that in `trunk`. This request will be reviewed by `cloud_analytics` group reviewers (assigned automatically).


### **Before creating pull-request <u>don't forget</u> to:**

 - Change the version of library in [setup.cfg](setup.cfg): change `0.0.*` for minor bug fixes; change `0.*.0` for adding features or significant changes. Read more about [Semantic Versioning](http://semver.org/)
 
 - Add a short description in change log of [README.md](README.md#change-log)
 
 - Check changes via `flake8` from root `clan_tools` path (contains setup.cfg):

```bash
flake8 ./
```

 - Check changes via `mypy` from root `clan_tools` path (contains setup.cfg):
```bash
mypy ./
```

 - After successful completing review process and merging differences to `trunk` add new version to [pypi-repository](https://wiki.yandex-team.ru/pypi/)
 - 
```bash
python setup.py sdist upload -r yandex
```
