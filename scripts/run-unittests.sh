#!/usr/bin/env bash

set -x

main () {
    cd "$(dirname "$(realpath "$0")")/.."
    if [ -n "$FSARCHS" ]; then
        local archs=()
        IFS=, read -ra archs <<< "$FSARCHS"
        for arch in "${archs[@]}" ; do
            run-tests "$arch"
        done
    else
        local os=$(uname -m -s)
        case $os in
            "Darwin arm64")
                run-tests darwin;;
            "Darwin x86_64")
                run-tests darwin;;
            "FreeBSD amd64")
                run-tests freebsd_amd64;;
            "Linux i686")
                run-tests linux32;;
            "Linux x86_64")
                run-tests linux64;;
            "OpenBSD amd64")
                run-tests openbsd_amd64;;
            *)
                echo "$0: Unknown OS architecture: $os" >&2
                exit 1
        esac
    fi
}

realpath () {
    if [ -x "/bin/realpath" ]; then
        /bin/realpath "$@"
    else
        python -c "import os.path, sys; print(os.path.realpath(sys.argv[1]))" \
               "$1"
    fi
}

run-test () {
    local arch=$1
    shift
    case $arch in
        linux32 | linux64)
            valgrind -q --leak-check=full --error-exitcode=1 "$@"
            ;;
        *)
            "$@"
            ;;
    esac
}

run-tests () {
    local arch=$1
    run-test $arch stage/$arch/build/test/avltest &&
    run-test $arch stage/$arch/build/test/bytearray_test &&
    run-test $arch stage/$arch/build/test/intset_test &&
    run-test $arch stage/$arch/build/test/charstr_normalization_test \
         unicode/NormalizationTest.txt &&
    run-test $arch stage/$arch/build/test/charstr_idna_test \
         idna/IdnaTestV2.txt &&
    run-test $arch stage/$arch/build/test/charstr_test &&
    run-test $arch stage/$arch/build/test/charstr_grapheme_test \
         unicode/auxiliary/GraphemeBreakTest.txt &&
    run-test $arch stage/$arch/build/test/base64_test &&
    run-test $arch stage/$arch/build/test/date_test &&
    run-test $arch stage/$arch/build/test/float_test &&
    run-test $arch stage/$arch/build/test/priorq_test
}

main "$@"
