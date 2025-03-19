#!/usr/bin/env bash


create_tarball() {
    for i in allCAs-*; do

        prj=$(basename "${i#*-}" .pem)
        echo "Make tarball for ${prj}"
        mkdir -p "${prj}/priv"
        cp allCAs-"${prj}".pem "${prj}"
        cp "${prj}.pem" "${prj}/priv"

        cd "${prj}" || exit 1
        for j in 1st 2nd 3rd; do
            openssl rand 48 > "priv/${j}.${prj}.key"
        done

        ${TAR} -czf secrets.tgz ./*
        rm -- *.pem
        rm -- priv/*.pem priv/*.key
        rmdir priv
        cd ..
        echo

    done
}

check_last_issuer_hash() {
    issuer_url="$(openssl x509 -noout -text -in "${1}" | awk -F : '/CA Issuers - URI/ {print $2":"$3; exit}')"

    # If url doesn't exist (e.g. YandexInternalRootCA doen't have url), when just return error
    if [ -z "${issuer_url}" ]; then
        issuer="$(openssl x509 -noout -issuer -in "${1}")"
        return 1
    fi

    curl -s "${issuer_url}" -o CA.cer

    subject_hash="$(openssl x509 -noout -in CA.cer -inform der -subject_hash)"

    if [ "${2}" != "${subject_hash}" ]; then
        echo "Check Root Failed: issuer not equal subject: ${2} - ${subject_hash}"
        exit 1
    fi

    test -f CA.cer && rm CA.cer

}

check_certs_chain() {
    counter=1
    for sub_cert in [0-9]*.pem; do
        if [ "${counter}" -eq 1 ]; then
            issuer_hash="$(openssl x509 -noout -in "${sub_cert}" -issuer_hash)"
            ((counter++))
        else
            next_subject_hash="$(openssl x509 -noout -in "${sub_cert}" -subject_hash)"

            if [ "${issuer_hash}" != "${next_subject_hash}" ]; then
                echo "Check intermediate files: issuer not equal subject: ${issuer_hash} - ${next_subject_hash}"
                exit 1
            fi

            next_issuer_hash="$(openssl x509 -noout -in "${sub_cert}" -issuer_hash)"
            issuer_hash="${next_issuer_hash}"
        fi
    done

    check_last_issuer_hash "${sub_cert}" "${issuer_hash}"
    if [ "${?}" -eq 1 ]; then
        echo "Empty CA Issuers URI: ${issuer}"
    fi

}

split_golem_cert() {
    for cert in *.pem; do
        awk '/-----BEGIN CERTIFICATE-----|-----BEGIN PRIVATE KEY-----/ {filename=sprintf("%04d.pem", NR)}; {print >filename}' "${cert}"
        mv 0001.pem "${cert}"

        check_certs_chain

        cat [0-9]*.pem > allCAs-"${cert}"
        rm [0-9]*.pem
    done
}

sanity_checks() {
    OS="$(uname)"
    if [ "${OS}" = "Linux" ]; then
        TAR="tar"
    elif [ "${OS}" = "Darwin" ]; then
        if [ -n "$(command -v gtar)" ]; then
            TAR="gtar"
        else
            echo "Please use gnu-tar instead of from system"
            echo "brew install gnu-tar"
            exit 0
        fi
    fi
}

main() {
    sanity_checks
    split_golem_cert
    create_tarball
}

main
