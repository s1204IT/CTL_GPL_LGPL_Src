#include "stk_ges_test.h"

int ges_test(stk_ges *ges)
{
    int i = 0;

    for (i = 0; i < 32; i ++)
    {
        if ( (ges->ori_u[i] > GESTURE_THRESHOLD_OUT) &&
             (ges->ori_d[i] > GESTURE_THRESHOLD_OUT) &&
             (ges->ori_l[i] > GESTURE_THRESHOLD_OUT) &&
             (ges->ori_r[i] > GESTURE_THRESHOLD_OUT) )
        {
            ges->u_first = ges->ori_u[i];
            ges->d_first = ges->ori_d[i];
            ges->l_first = ges->ori_l[i];
            ges->r_first = ges->ori_r[i];
            break;
        }
    }

    for (i = 31; i >= 0; i--)
    {
        if ( (ges->ori_u[i] > GESTURE_THRESHOLD_OUT) &&
             (ges->ori_d[i] > GESTURE_THRESHOLD_OUT) &&
             (ges->ori_l[i] > GESTURE_THRESHOLD_OUT) &&
             (ges->ori_r[i] > GESTURE_THRESHOLD_OUT) )
        {
            ges->u_last = ges->ori_u[i];
            ges->d_last = ges->ori_d[i];
            ges->l_last = ges->ori_l[i];
            ges->r_last = ges->ori_r[i];
            break;
        }
    }

    if( (ges->u_first == 0) || (ges->d_first == 0) || \
        (ges->l_first == 0) || (ges->r_first == 0) )
    {
        return 0;
    }

    ges->ud_ratio_first = ((ges->u_first - ges->d_first) * 100) / (ges->u_first + ges->d_first);
    ges->lr_ratio_first = ((ges->l_first - ges->r_first) * 100) / (ges->l_first + ges->r_first);
    ges->ud_ratio_last = ((ges->u_last - ges->d_last) * 100) / (ges->u_last + ges->d_last);
    ges->lr_ratio_last = ((ges->l_last - ges->r_last) * 100) / (ges->l_last + ges->r_last);
    ges->ud_delta = ges->ud_ratio_last - ges->ud_ratio_first;
    ges->lr_delta = ges->lr_ratio_last - ges->lr_ratio_first;
    ges->gesture_ud_delta_ += ges->ud_delta;
    ges->gesture_lr_delta_ += ges->lr_delta;

    /* Determine U/D gesture */
    if ( ges->gesture_ud_delta_ >= GESTURE_SENSITIVITY_1 )
    {
        ges->gesture_ud_count_ = 1;
    }
    else if ( ges->gesture_ud_delta_ <= -GESTURE_SENSITIVITY_1 )
    {
        ges->gesture_ud_count_ = -1;
    }
    else
    {
        ges->gesture_ud_count_ = 0;
    }

    /* Determine L/R gesture */
    if ( ges->gesture_lr_delta_ >= GESTURE_SENSITIVITY_1 )
    {
        ges->gesture_lr_count_ = 1;
    }
    else if ( ges->gesture_lr_delta_ <= -GESTURE_SENSITIVITY_1 )
    {
        ges->gesture_lr_count_ = -1;
    }
    else
    {
        ges->gesture_lr_count_ = 0;
    }

    /* Determine swipe direction */
    if ( (ges->gesture_ud_count_ == -1) && (ges->gesture_lr_count_ == 0) )
    {
        ges->gesture_motion_ = DIR_UP;
    }
    else if ( (ges->gesture_ud_count_ == 1) && (ges->gesture_lr_count_ == 0) )
    {
        ges->gesture_motion_ = DIR_DOWN;
    }
    else if ( (ges->gesture_ud_count_ == 0) && (ges->gesture_lr_count_ == 1) )
    {
        ges->gesture_motion_ = DIR_RIGHT;
    }
    else if ( (ges->gesture_ud_count_ == 0) && (ges->gesture_lr_count_ == -1) )
    {
        ges->gesture_motion_ = DIR_LEFT;
    }
    else if ( (ges->gesture_ud_count_ == -1) && (ges->gesture_lr_count_ == 1) )
    {
        if ( get_abs(ges->gesture_ud_delta_) > get_abs(ges->gesture_lr_delta_) )
        {
            ges->gesture_motion_ = DIR_UP;
        }
        else
        {
            ges->gesture_motion_ = DIR_RIGHT;
        }
    }
    else if ( (ges->gesture_ud_count_ == 1) && (ges->gesture_lr_count_ == -1) )
    {
        if ( get_abs(ges->gesture_ud_delta_) > get_abs(ges->gesture_lr_delta_) )
        {
            ges->gesture_motion_ = DIR_DOWN;
        }
        else
        {
            ges->gesture_motion_ = DIR_LEFT;
        }
    }
    else if ( (ges->gesture_ud_count_ == -1) && (ges->gesture_lr_count_ == -1) )
    {
        if ( get_abs(ges->gesture_ud_delta_) > get_abs(ges->gesture_lr_delta_) )
        {
            ges->gesture_motion_ = DIR_UP;
        }
        else
        {
            ges->gesture_motion_ = DIR_LEFT;
        }
    }
    else if ( (ges->gesture_ud_count_ == 1) && (ges->gesture_lr_count_ == 1) )
    {
        if ( get_abs(ges->gesture_ud_delta_) > get_abs(ges->gesture_lr_delta_) )
        {
            ges->gesture_motion_ = DIR_DOWN;
        }
        else
        {
            ges->gesture_motion_ = DIR_RIGHT;
        }
    }
    else
    {
        return 0;
    }
    return 0;
}

//MODULE_LICENSE("GPL");
