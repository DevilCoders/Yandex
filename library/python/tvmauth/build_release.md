# Steps:
1. Version up
2. Run task: https://sandbox.yandex-team.ru/scheduler/43565/view

# Adding a new platform/python

## Adding system python

### Check that python is available

In order to add a new platform, first you need to make sure system python for the specified python version and system exists.

```bash
cd <path to arcadia>/arcadia/library/python/tvmauth/so
ya make -r --target-platform <platform name> -DUSE_SYSTEM_PYTHON=1
```

{% note tip %}

If this compiles successfully, then you can skip the next step

{% endnote %}

### Add bundle

{% note warning %}

You need access to a machine with that exact platform in order to do this

{% endnote %}

<https://a.yandex-team.ru/arc/trunk/arcadia/build/platform/python/readme.md#darwin>

## Adding platform

{% note info %}

In order to add a new platform you will need to change the source code of the sandbox task above.

{% endnote %}

{% note alert %}

`pip` uses wheel names in order to determine whether they can be installed on a system

Please read <https://www.python.org/dev/peps/pep-0425> and <https://www.python.org/dev/peps/pep-0513> before proceeding. There be dragons...

{% endnote %}

{% note info %}

It doesn't look like the python version and platform are specified anywhere else in the wheel, except for in the name. You only need to change the name of the wheel, in order to fix a potential issue.

{% endnote %}

{% list tabs %}

- MacOS

  When a new MacOS is released, you most likely need not take action, you will only ever need to do this if a new architecture appears or Apple cuts backwards-compatibility.
  
  At the moment, when creating a wheel for macosx you need to specify the following in the name:
    ``` tvmauth-X-Y-Z-<python>-<abi>-<platform>.<maybe another platform>.<yet another platform>.whl ``` 
  
    - `python` - is the python version (cp38, cp310, etc.)
    - `abi` - can be left as `none` for macosx
    - `platform` - is the name of the platform, which looks like this: `macosx_<version>_<architecture>`, where `version` is the oldest compatible version of macosx and `architecture` is the same as in `--target-platform`

    {% note info %}

    The oldest compatible version of macosx depends on how far back backwards compatibility goes. For example, if you are running MacOS Bug Sur 11.4, the wheel can be `macosx_10_10_...`, while trying to install `macosx_10_0_...` will result in an error

    There is no need to specify every version under the sun, only the oldest you wish to support and any future ones than are incompatible with the oldest.

    {% endnote %}

    {% note info %}

    The architecture basically means the type of processor. I was not able to get a definitive list of correlations, but:
    - `intel` means your wheel supports x32 and x64 (since we compile for x64 only, this is not suitable).
    - `universal` and `universal2`, from what I understand, is basically for pure-python packages, so do not use it here.
    - `i386` is x32
    - `fat32` and `fat64` must be for PowerPC, by exclusion?
    - `x86_64` - is the intel x64 processor
    - `arm64` - is the M1 processor

    {% endnote %}

    {% note alert %}

    M1 processor support only appeared in MacOS 11, so pip will not recognize macosx_10_10_arm64, as this combination simply does not exist
  
    {% endnote %}

    So, for now the wheel names look as such:
    - `tvmauth-<version>-cp27-none-macosx_10_10_x86_64.whl`
    - `tvmauth-<version>-cp3x-none-macosx_10_10_x86_64.whl`
    - `tvmauth-<version>-cp3x-none-macosx_11_0_arm64.whl`

- Linux

  TBD

- Windows

  TBD

{% endlist %}
