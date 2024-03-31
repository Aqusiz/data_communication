#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

struct pk_list {
    long sn;
    double gentm, timeout;
    struct pk_list *link;
};
typedef struct pk_list DataQue;
DataQue *WQ_front, *WQ_rear;
DataQue *TransitQ_front, *TransitQ_rear;

struct ack_list {
    long sn;
    double ack_rtm;
    struct ack_list *link;
};
typedef struct ack_list AckQue;
AckQue *AQ_front, *AQ_rear;

long seq_n = 0;
long transit_pknum = 0;
long next_acksn = 0;
long t_pknum = 0;
double cur_tm, next_pk_gentm;
double t_delay = 0;

long N = 100000;
double timeout_len;
int W;
float a, t_pk, t_pro;
float lambda, p;

float make_random(void);
void pk_gen(double);
void suc_transmission(long);
void re_transmit(void);
void transmit_pk(void);
void receive_pk(long, double);
void enque_ack(long);
void cur_tm_update(void);
void print_performance_measure(void);

int main(int argc, char **argv)
{
    srand(time(0));
    // check the number of input parameters
    if (argc != 6) {
        printf("check parameters\n");
        printf("./go_back_N [W] [lambda] [t_pk] [p] [a]\n");
        return -1;
    }
    // check the values of input parameters
    if (((W = atol(argv[1])) == 0) ||
        ((lambda = atof(argv[2])) == 0) ||
        ((t_pk = atof(argv[3])) == 0) ||
        ((a = atof(argv[5])) == 0)) {
            printf("check parameters' value\n");
            printf("./go_back_N [W] [lambda] [t_pk] [p] [a]\n");
            return -1;
    }
    p = atof(argv[4]);
    // init
    t_pro = a * t_pk;
    timeout_len = 2*t_pro + t_pk + 0.3;
    WQ_front = WQ_rear = NULL;
    TransitQ_front = TransitQ_rear = NULL;
    AQ_front = AQ_rear = NULL;

    cur_tm = -log(make_random())/lambda;
    pk_gen(cur_tm);
    next_pk_gentm = cur_tm -log(make_random())/lambda;

    while (t_pknum < N) {
        // ACK process
        while(AQ_front != NULL) {
            if (AQ_front->ack_rtm <= cur_tm) {
                suc_transmission(AQ_front->sn);
            }
            else break;
        }
        // Timeout process
        if (TransitQ_front != NULL) {
            if (TransitQ_front->timeout <= cur_tm) {
                re_transmit();
            }
        }
        // generate new packet
        while (next_pk_gentm <= cur_tm) {
            pk_gen(next_pk_gentm);
            next_pk_gentm += -log(make_random())/lambda;
        }
        // HOL packet send/receive
        if ((WQ_front != NULL) && (transit_pknum < W)) {
            transmit_pk();
            receive_pk(TransitQ_rear->sn, TransitQ_rear->gentm);
        }
        // time update
        cur_tm_update();
    }
    print_performance_measure();
    return 0;
}

void pk_gen(double tm)
{
    DataQue *ptr;
    ptr = malloc(sizeof(DataQue));
    ptr->sn = seq_n;
    ptr->gentm = tm;
    ptr->link = NULL;
    seq_n++;
    // enque -> WQ_rear
    if (WQ_front == NULL) {
        WQ_front = ptr;
    }
    else {
        WQ_rear->link = ptr;
    }
    WQ_rear = ptr;
}

void suc_transmission(long sn)
{
    DataQue *ptr;
    AckQue *aptr;
    // deque <- TransitQ_front
    ptr = TransitQ_front;
    if(ptr->sn == sn) {
        // printf("successfully transmitted: %ld\n", sn);
        TransitQ_front = TransitQ_front->link;
        if (TransitQ_front == NULL)
            TransitQ_rear = NULL;
        free(ptr);
        // window size +1
        transit_pknum--;
    }
    // deque <- AQ_front
    aptr = AQ_front;
    AQ_front = aptr->link;
    if (AQ_front == NULL) AQ_rear = NULL;
    free(aptr);
}

void re_transmit(void)
{
    // printf("re transmit\n");
    // move TransitQ to WQ
    TransitQ_rear->link = WQ_front;
    if (WQ_front == NULL)
        WQ_rear = TransitQ_rear;
    WQ_front = TransitQ_front;
    TransitQ_front = TransitQ_rear = NULL;
    // reset window size
    transit_pknum = 0;
}

void transmit_pk(void)
{
    // printf("transmit: %ld\n", WQ_front->sn);
    DataQue *ptr;

    cur_tm += t_pk;
    WQ_front->timeout = cur_tm + timeout_len;
    // deque <- WQ_front
    ptr = WQ_front;
    WQ_front = WQ_front->link;
    if (WQ_front == NULL) WQ_rear = NULL;
    // enque -> TransitQ_rear
    if (TransitQ_front == NULL) TransitQ_front = ptr;
    else TransitQ_rear->link = ptr;
    ptr->link = NULL;
    TransitQ_rear = ptr;
    // window size -1
    transit_pknum++;
}

void receive_pk(long seqn, double gtm)
{
    if (make_random() > p) {
        if (next_acksn == seqn) {
            // printf("received: %ld\n", seqn);
            t_delay += cur_tm + t_pro - gtm;
            t_pknum++;
            next_acksn++;
            enque_ack(seqn);
        }
    }
}

void enque_ack(long seqn)
{
    AckQue *ack_ptr;
    ack_ptr = malloc(sizeof(AckQue));
    ack_ptr->sn = seqn;
    ack_ptr->ack_rtm = cur_tm + 2*t_pro;
    ack_ptr->link = NULL;

    if (AQ_front == NULL) AQ_front = ack_ptr;
    else AQ_rear->link = ack_ptr;
    AQ_rear = ack_ptr;
}

void cur_tm_update(void)
{
    double tm;

    if ((WQ_front != NULL) && (transit_pknum < W)) return;
    else {
        // take min value of (cur_tm, next_pk_gentm, ack_rtm, timeout)
        if (AQ_front == NULL)
            tm = next_pk_gentm;
        else if (AQ_front->ack_rtm < next_pk_gentm)
            tm = AQ_front->ack_rtm;
        else tm = next_pk_gentm;

        if (TransitQ_front != NULL)
            if (TransitQ_front->timeout < tm)
                tm = TransitQ_front->timeout;
        
        if (tm > cur_tm) cur_tm = tm;
    }
}

void print_performance_measure(void)
{
    double util;
    double m_delay;

    m_delay = t_delay / (double)t_pknum;
    util = (t_pknum * t_pk) / cur_tm;

    printf("N: %ld, timeout_len: %lf, W: %d, lambda: %f, t_pk: %f, p: %f, a: %f\n", N, timeout_len, W, lambda, t_pk, p, a);
    //printf("t_delay: %lf, cur_tm: %lf, seq_n: %ld\n", t_delay, cur_tm, seq_n);
    //printf("t_pk + 2 * t_pro: %f\n", t_pknum * (t_pk + 2 * t_pro));
    printf("m_delay: %lf, util: %lf\n", m_delay, util);
}

float make_random(void)
{
    float rn;
    rn = (float)rand() / (float) RAND_MAX;
    return rn;
}