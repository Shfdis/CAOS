#!/bin/bash
cd "$(dirname "$0")"
echo " Тест программы 4 "
gcc -Wall -Wextra -std=c11 -D_GNU_SOURCE -I../common program4_coordinator.c ../common/tournament.c ../common/coordinator_core.c ../common/ipc_utils.c ../common/signals.c ../common/utils.c -o program4_coordinator -lrt -lpthread
gcc -Wall -Wextra -std=c11 -D_GNU_SOURCE -I../common program4_player.c ../common/tournament.c ../common/player_core.c ../common/ipc_utils.c ../common/signals.c -o program4_player -lrt -lpthread
gcc -Wall -Wextra -std=c11 -D_GNU_SOURCE -I../common program4_observer.c ../common/tournament.c ../common/observer_core.c ../common/signals.c -o program4_observer -lrt -lpthread
if [ $? -eq 0 ]; then
    (echo "4"; sleep 1; echo "") | timeout 12 ./program4_coordinator > /tmp/p4c.log 2>&1 &
    sleep 1
    for i in {1..2}; do timeout 12 ./program4_observer > /tmp/p4o${i}.log 2>&1 & done
    sleep 1
    for i in {1..4}; do timeout 12 ./program4_player $i > /tmp/p4p${i}.log 2>&1 & done
    sleep 10
    echo "Координатор:"
    tail -5 /tmp/p4c.log | grep -E "(ТУРНИР|ПОБЕДИТЕЛЬ|Матч)" || tail -3 /tmp/p4c.log
    echo "Наблюдатель 1:"
    tail -3 /tmp/p4o1.log | grep -E "(НАБЛЮДАТЕЛЬ|ТУРНИР|ПОБЕДИТЕЛЬ)" || tail -2 /tmp/p4o1.log
    echo "Наблюдатель 2:"
    tail -3 /tmp/p4o2.log | grep -E "(НАБЛЮДАТЕЛЬ|ТУРНИР|ПОБЕДИТЕЛЬ)" || tail -2 /tmp/p4o2.log
    pkill -f program4_ 2>/dev/null
    echo "Тест завершен"
else
    echo "Ошибка компиляции"
fi

