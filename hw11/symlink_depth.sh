#!/bin/bash

TEMP_DIR="symlink_test_dir"

mkdir -p "$TEMP_DIR" || exit 1
cd "$TEMP_DIR" || exit 1

touch a || exit 1

depth=0
prev_name="a"
counter=0

while [ $counter -lt 100 ]; do
    if [ $counter -eq 0 ]; then
        curr_name="aa"
    else
        char=$(printf "\\$(printf '%03o' $((97 + counter)))")
        curr_name="a$char"
    fi

    ln -s "$prev_name" "$curr_name" || break

    if ! exec 3<"$curr_name" 2>/dev/null; then
        echo "$depth"
        exec 3<&-
        break
    fi
    exec 3<&-

    depth=$((depth + 1))
    prev_name="$curr_name"
    counter=$((counter + 1))
done

cd ..
rm -rf "$TEMP_DIR"

