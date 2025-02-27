#ifndef __PASCAL_SMOOTH_H__
#define __PASCAL_SMOOTH_H__


#define PSMOOTH_NUM_VALUES (10)

struct psmooth
{
    float values[PSMOOTH_NUM_VALUES];
    unsigned int current_index;
};

#ifdef __cplusplus
extern "C" {
#endif

void psmooth_init(struct psmooth* ctx);
void psmooth_push(struct psmooth* ctx, float value);
float psmooth_average(struct psmooth* ctx);

#ifdef __cplusplus
}
#endif


#endif 