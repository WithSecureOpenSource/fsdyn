#!/bin/bash -x

main () {
    cd "$(dirname "$(realpath "$0")")/.."
    local os=$(uname -s)
    if [ -n "$FSARCHS" ]; then
        local archs=()
        IFS=, read -ra archs <<< "$FSARCHS"
        for arch in "${archs[@]}" ; do
            run-tests "$arch"
        done
    elif [ x$os = xLinux ]; then
        local cpu=$(uname -m)
        if [ "x$cpu" == xx86_64 ]; then
            run-tests linux64
        elif [ "x$cpu" == xi686 ]; then
            run-tests linux32
        else
            echo "$0: Unknown CPU: $cpu" >&2
            exit 1
        fi
    elif [ "x$os" = xDarwin ]; then
        run-tests darwin
    else
        echo "$0: Unknown OS architecture: $os" >&2
        exit 1
    fi
}

realpath () {
    if [ -x "/bin/realpath" ]; then
        /bin/realpath "$@"
    else
        python -c "import os.path, sys; print os.path.realpath(sys.argv[1])" \
               "$1"
    fi
}

run-test () {
    local arch=$1
    shift
    case $arch in
        darwin)
            "$@"
            ;;
        *)
            valgrind -q --leak-check=full --error-exitcode=1 "$@"
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
    run-test $arch stage/$arch/build/test/charstr_test &&
    run-test $arch stage/$arch/build/test/base64_test &&
    run-test $arch stage/$arch/build/test/date_test &&
    run-test $arch stage/$arch/build/test/float_test &&
    run-test $arch stage/$arch/build/test/priorq_test
}

main "$@"
