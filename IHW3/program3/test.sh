#!/bin/bash
cd "$(dirname "$0")"
echo " Тест программы 3 "
gcc -Wall -Wextra -std=c11 -D_GNU_SOURCE -I../common program3_coordinator.c ../common/tournament.c ../common/coordinator_core.c ../common/ipc_utils.c ../common/signals.c ../common/utils.c -o program3_coordinator -lrt -lpthread
gcc -Wall -Wextra -std=c11 -D_GNU_SOURCE -I../common program3_player.c ../common/tournament.c ../common/player_core.c ../common/ipc_utils.c ../common/signals.c -o program3_player -lrt -lpthread
gcc -Wall -Wextra -std=c11 -D_GNU_SOURCE -I../common program3_observer.c ../common/tournament.c ../common/observer_core.c ../common/signals.c -o program3_observer -lrt -lpthread
if [ $? -eq 0 ]; then
    (echo "4"; sleep 1; echo "") | timeout 12 ./program3_coordinator > /tmp/p3c.log 2>&1 &
    sleep 1
    timeout 12 ./program3_observer > /tmp/p3o.log 2>&1 &
    sleep 1
    for i in {1..4}; do timeout 12 ./program3_player $i > /tmp/p3p${i}.log 2>&1 & done
    sleep 10
    echo "Координатор:"
    tail -5 /tmp/p3c.log | grep -E "(ТУРНИР|ПОБЕДИТЕЛЬ|Матч)" || tail -3 /tmp/p3c.log
    echo "Наблюдатель:"
    tail -5 /tmp/p3o.log | grep -E "(НАБЛЮДАТЕЛЬ|ТУРНИР|ПОБЕДИТЕЛЬ)" || tail -3 /tmp/p3o.log
    pkill -f program3_ 2>/dev/null
    echo "Тест завершен"
else
    echo "Ошибка компиляции"
fi

