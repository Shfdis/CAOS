#!/bin/bash
cd "$(dirname "$0")"
echo "Тест программы 2 "
gcc -Wall -Wextra -std=c11 -D_GNU_SOURCE -I../common program2_coordinator.c ../common/tournament.c ../common/coordinator_core.c ../common/ipc_utils.c ../common/signals.c ../common/utils.c -o program2_coordinator -lrt -lpthread
gcc -Wall -Wextra -std=c11 -D_GNU_SOURCE -I../common program2_player.c ../common/tournament.c ../common/player_core.c ../common/ipc_utils.c ../common/signals.c -o program2_player -lrt -lpthread
if [ $? -eq 0 ]; then
    (echo "4"; sleep 1; echo "") | timeout 12 ./program2_coordinator > /tmp/p2c.log 2>&1 &
    sleep 1
    for i in {1..4}; do timeout 12 ./program2_player $i > /tmp/p2p${i}.log 2>&1 & done
    sleep 10
    echo "Координатор:"
    tail -5 /tmp/p2c.log | grep -E "(ТУРНИР|ПОБЕДИТЕЛЬ|Матч)" || tail -3 /tmp/p2c.log
    pkill -f program2_ 2>/dev/null
    echo "Тест завершен"
else
    echo "Ошибка компиляции"
fi

