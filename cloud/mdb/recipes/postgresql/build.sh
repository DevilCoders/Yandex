#!/bin/bash

set -xe

POSTGRE_FULL_VERSION=${1:-12.10}
POSTGRE_FILE_NAME="postgresql-${POSTGRE_FULL_VERSION}.tar.gz"
PYTHON_VERSION=3.9.10
PYTHON_FILE_NAME=Python-${PYTHON_VERSION}.tgz
POSTGRE_DIR_NAME=postgresql-${POSTGRE_FULL_VERSION}
PYTHON_DIR_NAME=Python-${PYTHON_VERSION}
GETTEXT_VERSION=0.21
GETTEXT_FILE_NAME=gettext-${GETTEXT_VERSION}.tar.gz
GETTEXT_DIR_NAME=gettext-${GETTEXT_VERSION}

TARGET_DIR=$(pwd)/pg
rm -rf "${TARGET_DIR}" "${POSTGRE_DIR_NAME}" "${PYTHON_DIR_NAME}" "${GETTEXT_DIR_NAME}"
mkdir "${TARGET_DIR}"

if [ "$(uname)" = "Darwin" ];
then
    # psycopg2.errors.UndefinedFile: could not load library "/...content/pg/lib/postgresql/plpython3.so": dlopen(/...content/pg/lib/postgresql/plpython3.so, 10): Symbol not found: _preadv
    # Referenced from: /.../pg/lib/libpython3.9.dylib (which was built for Mac OS X 12.2)

    export MACOSX_DEPLOYMENT_TARGET=10.15

    # OS X, doesn't include gettext
    # build and put it into the recipe instead of relying on brew.
    wget -c "https://ftp.gnu.org/pub/gnu/gettext/${GETTEXT_FILE_NAME}"
    tar -zxf "${GETTEXT_FILE_NAME}"
    cd "${GETTEXT_DIR_NAME}"
    ./configure --disable-rpath --prefix="${TARGET_DIR}"
    make install

    cd ..
fi

wget -c "https://www.python.org/ftp/python/${PYTHON_VERSION}/${PYTHON_FILE_NAME}"
tar -zxf "${PYTHON_FILE_NAME}"
cd "${PYTHON_DIR_NAME}"
./configure --prefix="${TARGET_DIR}" --enable-optimizations --enable-shared
make install

cd ..

# We should be able to run our Python
export LD_LIBRARY_PATH="${TARGET_DIR}/lib:${LD_LIBRARY_PATH}"
export DYLD_FALLBACK_LIBRARY_PATH="${TARGET_DIR}/lib:${DYLD_FALLBACK_LIBRARY_PATH}"

wget -c "https://ftp.postgresql.org/pub/source/v${POSTGRE_FULL_VERSION}/${POSTGRE_FILE_NAME}"
tar -zxf "${POSTGRE_FILE_NAME}"
cd "${POSTGRE_DIR_NAME}"
./configure --disable-rpath --with-python --with-uuid=e2fs --prefix="${TARGET_DIR}"  PYTHON="${TARGET_DIR}/bin/python3"
make install

cd contrib
make PG_CONFIG="${TARGET_DIR}/bin/pg_config" install


git clone --branch=v1.7.6 https://github.com/okbob/plpgsql_check.git
git clone --branch=plproxy_2_9 https://github.com/plproxy/plproxy.git
# we need to revert commit which breaks plproxy build on MacOS
cd plproxy && git revert --no-edit d10fef0e && cd -
git clone --branch=v4.2.2 https://github.com/pgpartman/pg_partman.git
git clone --branch=master https://github.com/dsarafan/repl_mon.git

for subdir in plpgsql_check repl_mon pg_partman plproxy
do
    make PG_CONFIG="${TARGET_DIR}/bin/pg_config" -C ${subdir} install
done

for file in $(find "${TARGET_DIR}" -type l);
do
    src="$(readlink -f "${file}" 2>/dev/null || echo "$(dirname "${file}")/$(readlink "${file}")")"
    rm -f "${file}"
    cp "${src}" "${file}"
done

# ~/C/pg-recipe $ ya upload --ttl=inf pg --http
# Error: Could not upload pg
# ValueError: Amount of files to be uploaded (12355) bigger than allowed (10000)
# Could not upload pg: Amount of files to be uploaded (12355) bigger than allowed (10000)

cd "$TARGET_DIR"
rm -rf share/doc share/man include
find . -type d -name '__pycache__' | xargs rm -rf

echo "Now you should run 'ya upload --ttl=inf pg'"
