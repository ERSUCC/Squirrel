#!/bin/sh

set -e

if [ $# -lt 1 ]; then
    echo "You must provide the path to the Contents directory of the target app."
    exit 1
fi

SQUIRREL="$1/MacOS/squirrel"

install_name_tool -add_rpath "@executable_path/../Frameworks" "$SQUIRREL"

IFS=$'\n'

for LIB in $(otool -LX "$SQUIRREL" | tail -n +2 | grep -v -e /usr/ -e /System/ | grep -io /.*\.dylib); do
    install_name_tool -change "$LIB" @rpath/$(echo $LIB | grep -io [^/]*\.dylib) "$SQUIRREL"
done

for FILE in "$1/Frameworks"/*; do
    install_name_tool -id @rpath/$(otool -DX "$FILE" | grep -io [^/]*\.dylib) "$FILE"

    for LIB in $(otool -LX "$FILE" | tail -n +2 | grep -v -e /usr/ -e /System/ | grep -io "/.*\.dylib"); do
        install_name_tool -change "$LIB" @rpath/$(echo $LIB | grep -io [^/]*\.dylib) "$FILE"
    done
done
