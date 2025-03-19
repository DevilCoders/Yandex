pg-build-deps:
    pkg.installed:
        - pkgs:
            - libc6-dbg
            - libperl-dev
            - libipc-run-perl
            - libtap-parser-sourcehandler-pgtap-perl
            - tcl
            - tcl-tls
            - tcl8.6-dev
            - libedit-dev
            - libssl-dev
            - zlib1g-dev
            - libpam0g-dev
            - libxml2-dev
            - krb5-multidev
            - libldap2-dev
            - libselinux1-dev
            - libxslt1-dev
            - uuid-dev
            - python-dev
            - python3-dev
            - bison
            - flex
            - openjade
            - docbook-dsssl
            - docbook-xsl
            - docbook
            - opensp
            - xsltproc
            - gettext
        - prereq_in:
            - cmd: repositories-ready

clang-pkgs:
    pkg.installed:
        - pkgs:
            - clang-14
            - clang-format-14
            - clang-tidy-14
            - clang-tools-14
            - libc++-14-dev
            - libc++1-14
            - libc++abi-14-dev
            - libc++abi1-14
            - lld-14
            - lldb-14
            - llvm-14
            - llvm-14-runtime
            - llvm-14-tools
            - lcov
        - prereq_in:
            - cmd: repositories-ready

cmake-pkgs:
    pkg.installed:
        - pkgs:
            - cmake: 3.22.1-0kitware1ubuntu18.04.1
            - cmake-data: 3.22.1-0kitware1ubuntu18.04.1
        - prereq_in:
            - cmd: repositories-ready

include:
    - components.arcadia-build-node
    - components.repositories.apt.kitware
    - components.repositories.apt.llvm-org

/usr/local/bin/build-stable-pg.sh:
    file.absent
