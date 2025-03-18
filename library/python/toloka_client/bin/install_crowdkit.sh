set -e

cd -P `dirname $0`

if [[ `uname -s` == "Darwin" && `uname -m` == 'arm64' ]]; then
    echo "MacOS with Apple Silicon detected"

    if ! brew ls --versions rust > /dev/null ; then
        echo "Installing rust compiler"
        brew install rust
    fi

    if ! brew ls --versions openblas > /dev/null ; then
        echo "Installing openblas"
        brew install openblas
    fi

    OPENBLAS="$(brew --prefix openblas)" python -m pip install \
        --no-binary :all: \
        ../../stubmaker \
        ../../crowd-kit \

else

    python -m pip install \
        ../../stubmaker \
        ../../crowd-kit \

fi
