/* ir.h — cabinet IR convolution. */
#ifndef TF_IR_H
#define TF_IR_H
#define IR_MAX_TAPS 4096            /* 85 ms @48k */
typedef struct { float h[IR_MAX_TAPS]; float z[IR_MAX_TAPS]; int n, pos; } ir_conv;
void ir_set(ir_conv *c, const float *taps, int ntaps);
void ir_process(ir_conv *c, float *x, int n);
#endif
