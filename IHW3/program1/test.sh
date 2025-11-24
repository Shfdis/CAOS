#!/bin/bash
cd "$(dirname "$0")"
echo "Тест программы 1"
gcc -Wall -Wextra -std=c11 -D_GNU_SOURCE -I../common program1.c ../common/tournament.c -o program1 -lrt -lpthread
if [ $? -eq 0 ]; then
    echo "4" | timeout 10 ./program1 2>&1 | head -20
    echo ""
    echo "Тест завершен"
else
    echo "Ошибка компиляции"
fi

