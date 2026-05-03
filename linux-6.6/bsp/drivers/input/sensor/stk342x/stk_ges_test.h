#ifndef STK_GES_TEST
#define STK_GES_TEST

#include <linux/types.h>
#include <linux/module.h>

enum
{
    DIR_NONE,
    DIR_LEFT,
    DIR_RIGHT,
    DIR_UP,
    DIR_DOWN,
    DIR_NEAR,
    DIR_FAR,
    DIR_ALL
};

#define get_abs(x)              ((x < 0)? (-x):(x))
#define GESTURE_THRESHOLD_OUT 50
#define GESTURE_SENSITIVITY_1 50

typedef struct ges_t
{
    uint16_t ori_u[32];
    uint16_t ori_d[32];
    uint16_t ori_l[32];
    uint16_t ori_r[32];
    uint16_t u_first;
    uint16_t d_first;
    uint16_t l_first;
    uint16_t r_first;
    uint16_t u_last;
    uint16_t d_last;
    uint16_t l_last;
    uint16_t r_last;
    int ori_cnt;
    int ud_ratio_first;
    int lr_ratio_first;
    int ud_ratio_last;
    int lr_ratio_last;
    int ud_delta;
    int lr_delta;
    int gesture_ud_delta_;
    int gesture_lr_delta_;
    int gesture_ud_count_;
    int gesture_lr_count_;
    int gesture_motion_;
} stk_ges;

int ges_test(stk_ges *ges);

#endif
