#include "observer_core.h"
#include "tournament.h"
#include <stdio.h>
#include <mqueue.h>
#include <stdlib.h>

static ssize_t mq_recv(void *ctx, char *buf, size_t size) {
    mqd_t mq = *(mqd_t*)ctx;
    return mq_receive(mq, buf, size, NULL);
}

int main() {
    mqd_t mq = mq_open(MQ_NAME, O_RDONLY);
    if (mq == (mqd_t)-1) {
        perror("mq_open failed");
        printf("Убедитесь, что координатор запущен первым\n");
        return 1;
    }
    
    int result = observer_loop("Наблюдатель", &mq, mq_recv);
    mq_close(mq);
    return result;
}
