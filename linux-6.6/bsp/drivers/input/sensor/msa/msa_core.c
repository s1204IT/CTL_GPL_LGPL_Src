/* Core file for MEMS 3-Axis Accelerometer's driver.
 *
 * msa_core.c - Linux kernel modules for 3-Axis Accelerometer
 *
 * Copyright (C) 2007-2016 MEMS Sensing Technology Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "msa_core.h"
#include "../../init-input.h"
#include "msa_cust.h"

#define MSA_REG_ADDR(REG) ((REG)&0xFF)

#define MSA_OFFSET_THRESHOLD 20
#define PEAK_LVL 800
#define STICK_LSB 2000
#define AIX_HISTORY_SIZE 20

typedef struct reg_obj_s {

	short addr;
	unsigned char mask;
	unsigned char value;

} reg_obj_t;

struct gsensor_data_fmt_s {

	unsigned char msbw;
	unsigned char lsbw;
	unsigned char endian; /* 0: little endian; 1: big endian */
};

struct gsensor_data_obj_s {

#define MSA_DATA_LEN 6
	reg_obj_t data_sect[MSA_DATA_LEN];
	struct gsensor_data_fmt_s data_fmt;
};

struct gsensor_obj_s {

	char asic[10];

	reg_obj_t chip_id;
	reg_obj_t mod_id;
	reg_obj_t soft_reset;
	reg_obj_t power;

#define MSA_INIT_SECT_LEN 11
#define MSA_OFF_SECT_LEN MSA_OFFSET_LEN
#define MSA_ODR_SECT_LEN 3

	reg_obj_t init_sect[MSA_INIT_SECT_LEN];
	reg_obj_t offset_sect[MSA_OFF_SECT_LEN];
	reg_obj_t odr_sect[MSA_ODR_SECT_LEN];

	struct gsensor_data_obj_s data;

	int (*calibrate)(MSA_HANDLE handle, int z_dir);
	int (*auto_calibrate)(MSA_HANDLE handle, int xyz[3]);
	int (*int_ops)(MSA_HANDLE handle, msa_int_ops_t *ops);
	int (*get_reg_data)(MSA_HANDLE handle, char *buf);
};

struct gsensor_drv_s {

	struct general_op_s *method;

	struct gsensor_obj_s *obj;
};

typedef enum _asic_type {
	ASIC_NONE,
	ASIC_2511,
	ASIC_2512B,
} asic_type;

typedef enum _mems_type {
	MEMS_NONE,
	MEMS_T4,
	MEMS_T9,
	MEMS_TV03,
		MEMS_RTO3,
} mems_type;

typedef enum _package_type {
	PACKAGE_NONE,
	PACKAGE_2X2_12PIN,
} package_type;

struct chip_info_s {
	unsigned char reg_value;
	package_type package;
	asic_type asic;
	mems_type mems;
};

struct chip_info_s gsensor_chip_info;

static struct chip_info_s msa_chip_info_list[] = {
    {0x00, PACKAGE_2X2_12PIN, ASIC_2512B, MEMS_TV03},
    {0x01, PACKAGE_2X2_12PIN, ASIC_2511, MEMS_T4},
    {0x02, PACKAGE_2X2_12PIN, ASIC_2511, MEMS_T9},
};

#define MSA_NSA_INIT_SECTION                                                   \
  {                                                                            \
	{NSA_REG_G_RANGE, 0xFF, 0x01}, {NSA_REG_POWERMODE_BW, 0xFF, 0x1e},         \
		{NSA_REG_ODR_AXIS_DISABLE, 0xFF, 0x07},                                \
		{NSA_REG_INTERRUPT_SETTINGS2, 0xFF, 0x00},                             \
		{NSA_REG_INTERRUPT_MAPPING2, 0xFF, 0x00},                              \
		{NSA_REG_ENGINEERING_MODE, 0xFF, 0x83},                                \
		{NSA_REG_ENGINEERING_MODE, 0xFF, 0x69},                                \
		{NSA_REG_ENGINEERING_MODE, 0xFF, 0xBD},                                \
		{NSA_REG_INT_PIN_CONFIG, 0x0F, 0x05}, {-1, 0x00, 0x00}, {              \
      -1, 0x00, 0x00                                                           \
    }                                                                          \
  }

#define MSA_NSA_OFFSET_SECTION                                                 \
  {                                                                            \
    {NSA_REG_COARSE_OFFSET_TRIM_X, 0xFF, 0x00},                                \
		{NSA_REG_COARSE_OFFSET_TRIM_Y, 0xFF, 0x00},                            \
		{NSA_REG_COARSE_OFFSET_TRIM_Z, 0xFF, 0x00},                            \
		{NSA_REG_FINE_OFFSET_TRIM_X, 0xFF, 0x00},                              \
		{NSA_REG_FINE_OFFSET_TRIM_Y, 0xFF, 0x00},                              \
		{NSA_REG_FINE_OFFSET_TRIM_Z, 0xFF, 0x00},                              \
		{NSA_REG_CUSTOM_OFFSET_X, 0xFF, 0x00},                                 \
		{NSA_REG_CUSTOM_OFFSET_Y, 0xFF, 0x00},                                 \
		{NSA_REG_CUSTOM_OFFSET_Z, 0xFF, 0x00},                                 \
  }

#define MSA_NSA_ODR_SECTION                                                    \
  {                                                                            \
    {NSA_REG_ODR_AXIS_DISABLE, 0x0F, 0x06},                                    \
		{NSA_REG_ODR_AXIS_DISABLE, 0x0F, 0x07}, {                              \
      NSA_REG_ODR_AXIS_DISABLE, 0x0F, 0x08                                     \
    }                                                                          \
  }

#define MSA_NSA_DATA_SECTION                                                   \
  {                                                                            \
	{{NSA_REG_ACC_X_LSB, 0xFF, 0x00}, {NSA_REG_ACC_X_MSB, 0xFF, 0x00},         \
	 {NSA_REG_ACC_Y_LSB, 0xFF, 0x00}, {NSA_REG_ACC_Y_MSB, 0xFF, 0x00},         \
     {NSA_REG_ACC_Z_LSB, 0xFF, 0x00}, {NSA_REG_ACC_Z_MSB, 0xFF, 0x00}},        \
    {                                                                          \
      8, 5, 0                                                                  \
    }                                                                          \
  }

static int NSA_NTO_calibrate(MSA_HANDLE handle, int z_dir);
static int NSA_NTO_auto_calibrate(MSA_HANDLE handle, int xyz[3]);
static int NSA_interrupt_ops(MSA_HANDLE handle, msa_int_ops_t *ops);
static int NSA_get_reg_data(MSA_HANDLE handle, char *buf);

#define MSA_NSA_NTO                                                            \
  {                                                                            \
    "NSA_NTO", {NSA_REG_WHO_AM_I, 0xFF, 0x13},                                 \
		{NSA_REG_FIFO_CTRL, 0xFF, 0x20}, {NSA_REG_SPI_I2C, 0x24, 0x24},        \
		{NSA_REG_POWERMODE_BW, 0xFF, 0xC0}, MSA_NSA_INIT_SECTION,              \
		MSA_NSA_OFFSET_SECTION, MSA_NSA_ODR_SECTION, MSA_NSA_DATA_SECTION,     \
		NSA_NTO_calibrate, NSA_NTO_auto_calibrate, NSA_interrupt_ops,          \
		NSA_get_reg_data,                                                      \
  }
/**************************************************************** COMMON
 * ***************************************************************************/
#define MSA_GSENSOR_SCHEME MSA_SUPPORT_CHIP_LIST

static int z_offset, x_offset, y_offset;

/* this level can be modified while runtime through system attribute */
int Log_level = DEBUG_ERR;
int gsensor_mod = -1;  /* Initial value */
int gsensor_type = -1; /* Initial value */
static struct gsensor_obj_s msa_gsensor[] = {MSA_GSENSOR_SCHEME};
struct gsensor_drv_s msa_gsensor_drv;

#define MI_DATA(format, ...)                                                   \
  do {                                                                         \
    if (DEBUG_DATA & Log_level) {                                              \
      printk(MI_TAG format "\n", ##__VA_ARGS__);                               \
    }                                                                          \
  } while (0)
#define MI_MSG(format, ...)                                                    \
  do {                                                                         \
    if (DEBUG_MSG & Log_level) {                                               \
      printk(MI_TAG format "\n", ##__VA_ARGS__);                               \
    }                                                                          \
  } while (0)
#define MI_ERR(format, ...)                                                    \
  do {                                                                         \
    if (DEBUG_ERR & Log_level) {                                               \
      printk(MI_TAG format "\n", ##__VA_ARGS__);                               \
    }                                                                          \
  } while (0)
#define MI_FUN                                                                 \
  do {                                                                         \
    if (DEBUG_FUNC & Log_level) {                                              \
      printk(MI_TAG "%s is called, line: %d\n", __FUNCTION__, __LINE__);       \
    }                                                                          \
  } while (0)
#define MI_ASSERT(expr)                                                        \
  do {                                                                         \
    if (!(expr)) {                                                             \
      printk("Assertion failed! %s,%d,%s,%s\n", __FILE__, __LINE__, __func__,  \
			 #expr);                                                           \
    }                                                                          \
  } while (0)

/*
#ifndef MTK_ANDROID_M
#define abs(x) ({ long __x = (x); (__x < 0) ? -__x : __x; })
#endif
*/


int squareRoot(int val)
{
  int r = 0;
  int shift;

  if (val < 0) {
    return 0;
  }

  for (shift = 0; shift < 32; shift += 2) {
    int x = 0x40000000l >> shift;
    if (x + r <= val) {
      val -= x + r;
      r = (r >> 1) | x;
    } else {
      r = r >> 1;
    }
  }

  return r;
}


int msa_register_read(MSA_HANDLE handle, short addr, unsigned char *data)
{
  int res = 0;

  res = msa_gsensor_drv.method->smi.read(handle, MSA_REG_ADDR(addr), data);

  return res;
}

int msa_register_read_continuously(MSA_HANDLE handle, short addr,
									unsigned char count, unsigned char *data)
{
  int res = 0;

  res = (count == msa_gsensor_drv.method->smi.read_block(
									handle, MSA_REG_ADDR(addr), count, data))
			? 0
			: 1;

  return res;
}

int msa_register_write(MSA_HANDLE handle, short addr, unsigned char data)
{
  int res = 0;

  res = msa_gsensor_drv.method->smi.write(handle, MSA_REG_ADDR(addr), data);

  return res;
}

int msa_register_mask_write(MSA_HANDLE handle, short addr, unsigned char mask,
										unsigned char data)
{
  int res = 0;
  unsigned char tmp_data;

  res = msa_register_read(handle, addr, &tmp_data);
  if (res) {
    return res;
  }

  tmp_data &= ~mask;
  tmp_data |= data & mask;
  res = msa_register_write(handle, addr, tmp_data);

  return res;
}

static int msa_read_raw_data(MSA_HANDLE handle, short *x, short *y, short *z)
{
  unsigned char tmp_data[6] = {0};
  int tmp_x = 0, tmp_y = 0, tmp_z = 0;

  if (msa_register_read_continuously(
			handle, msa_gsensor_drv.obj[gsensor_mod].data.data_sect[0].addr, 6,
			tmp_data) != 0) {
    MI_ERR("i2c block read failed\n");
    return -1;
  }

  *x = ((short)(tmp_data[1]
			<< msa_gsensor_drv.obj[gsensor_mod].data.data_fmt.msbw |
			tmp_data[0])) >>
       (8 - msa_gsensor_drv.obj[gsensor_mod].data.data_fmt.lsbw);
  *y = ((short)(tmp_data[3]
			<< msa_gsensor_drv.obj[gsensor_mod].data.data_fmt.msbw |
				tmp_data[2])) >>
       (8 - msa_gsensor_drv.obj[gsensor_mod].data.data_fmt.lsbw);
  *z = ((short)(tmp_data[5]
			<< msa_gsensor_drv.obj[gsensor_mod].data.data_fmt.msbw |
				tmp_data[4])) >>
       (8 - msa_gsensor_drv.obj[gsensor_mod].data.data_fmt.lsbw);

  MI_DATA("msa321_raw: x=%d, y=%d, z=%d", *x, *y, *z);

  tmp_x = *x;
  tmp_y = *y;
  tmp_z = *z;

  msa_temp_calibrate(&tmp_x, &tmp_y, &tmp_z);

  *x = tmp_x;
  *y = tmp_y;
  *z = tmp_z;

  MI_DATA("msa321_raw_calibrate: x=%d, y=%d, z=%d", *x, *y, *z);

  return 0;
}

static int remap[8][4] = {{0, 0, 0, 0}, {0, 1, 0, 1}, {1, 1, 0, 0},
							{1, 0, 0, 1}, {1, 0, 1, 0}, {0, 0, 1, 1},
							{0, 1, 1, 0}, {1, 1, 1, 1}};

int msa_direction_remap(short *x, short *y, short *z, int direction)
{
  short temp = 0;

  *x = *x - ((*x) * remap[direction][0] * 2);
  *y = *y - ((*y) * remap[direction][1] * 2);
  *z = *z - ((*z) * remap[direction][2] * 2);

  if (remap[direction][3]) {
    temp = *x;
    *x = *y;
    *y = temp;
  }

  if (remap[direction][2])
    return -1;

  return 1;
}

int msa_read_data(MSA_HANDLE handle, short *x, short *y, short *z)
{
  int rst = 0;

  rst = msa_read_raw_data(handle, x, y, z);

  if (rst != 0) {
    MI_ERR("msa_read_raw_data failed, rst = %d", rst);
    return rst;
  }

  return 0;
}

int cycle_read_xyz(MSA_HANDLE handle, int *x, int *y, int *z, int ncycle)
{
  unsigned int j = 0;
  short raw_x, raw_y, raw_z;

  *x = *y = *z = 0;

  for (j = 0; j < ncycle; j++) {
    raw_x = raw_y = raw_z = 0;
    msa_read_raw_data(handle, &raw_x, &raw_y, &raw_z);

    (*x) += raw_x;
    (*y) += raw_y;
    (*z) += raw_z;

    msa_gsensor_drv.method->msdelay(5);
  }

  (*x) /= ncycle;
  (*y) /= ncycle;
  (*z) /= ncycle;

  return 0;
}

int msa_read_offset(MSA_HANDLE handle, unsigned char *offset)
{
  int i, res = 0;

  for (i = 0; i < MSA_OFF_SECT_LEN; i++) {
    if (msa_gsensor_drv.obj[gsensor_mod].offset_sect[i].addr < 0) {
      break;
    }

    res = msa_register_read(
		handle, msa_gsensor_drv.obj[gsensor_mod].offset_sect[i].addr,
		&offset[i]);
    if (res != 0) {
      return res;
    }
  }

  return res;
}

int msa_write_offset(MSA_HANDLE handle, unsigned char *offset)
{
  int i, res = 0;

  for (i = 0; i < MSA_OFF_SECT_LEN; i++) {
    if (msa_gsensor_drv.obj[gsensor_mod].offset_sect[i].addr < 0) {
      break;
    }

    res = msa_register_write(
		handle, msa_gsensor_drv.obj[gsensor_mod].offset_sect[i].addr,
		offset[i]);
    if (res != 0) {
      return res;
    }
  }

  return res;
}


static int NSA_NTO_calibrate(MSA_HANDLE handle, int z_dir)
{
  int result = 0;

  return result;
}

static int NSA_NTO_auto_calibrate(MSA_HANDLE handle, int xyz[3])
{
  int result = 0;

  return result;
}

int msa_calibrate(MSA_HANDLE handle, int z_dir)
{
  int res = 0;

  return res;
}


static int NSA_interrupt_ops(MSA_HANDLE handle, msa_int_ops_t *ops)
{
  switch (ops->type) {
  case INTERRUPT_OP_INIT:

    /* latch */
    msa_register_mask_write(handle, NSA_REG_INT_LATCH, 0x0f,
						ops->data.init.latch);
    /* active level & output mode */
    msa_register_mask_write(
		handle, NSA_REG_INT_PIN_CONFIG, 0x0f,
		ops->data.init.level | (ops->data.init.pin_mod << 1) |
			(ops->data.init.level << 2) | (ops->data.init.pin_mod << 3));
    break;

  case INTERRUPT_OP_ENABLE:
    switch (ops->data.int_src) {
    case INTERRUPT_ACTIVITY:
      /* Enable active interrupt */
      msa_register_mask_write(handle, NSA_REG_INTERRUPT_SETTINGS1, 0x07, 0x07);
      break;
    case INTERRUPT_CLICK:
      /* Enable single and double tap detect */
      msa_register_mask_write(handle, NSA_REG_INTERRUPT_SETTINGS1, 0x30, 0x30);
      break;
    }
    break;

  case INTERRUPT_OP_CONFIG:
    switch (ops->data.cfg.int_src) {
    case INTERRUPT_ACTIVITY:
      msa_register_write(handle, NSA_REG_ACTIVE_THRESHOLD,
							ops->data.cfg.int_cfg.act.threshold);
      msa_register_mask_write(handle, NSA_REG_ACTIVE_DURATION, 0x03,
							ops->data.cfg.int_cfg.act.duration);

      /* Int mapping */
      if (ops->data.cfg.pin == INTERRUPT_PIN1) {
		msa_register_mask_write(handle, NSA_REG_INTERRUPT_MAPPING1, (1 << 2),
								(1 << 2));
      } else if (ops->data.cfg.pin == INTERRUPT_PIN2) {
		msa_register_mask_write(handle, NSA_REG_INTERRUPT_MAPPING3, (1 << 2),
								(1 << 2));
      }
      break;

    case INTERRUPT_CLICK:

      msa_register_mask_write(handle, NSA_REG_TAP_THRESHOLD, 0x1f,
							ops->data.cfg.int_cfg.clk.threshold);
      msa_register_mask_write(handle, NSA_REG_TAP_DURATION,
								(0x03 << 5) | (0x07),
								(ops->data.cfg.int_cfg.clk.quiet_time << 7) |
									(ops->data.cfg.int_cfg.clk.click_time << 6) |
										(ops->data.cfg.int_cfg.clk.window));

      if (ops->data.cfg.pin == INTERRUPT_PIN1) {
			msa_register_mask_write(handle, NSA_REG_INTERRUPT_MAPPING1, 0x30, 0x30);
      } else if (ops->data.cfg.pin == INTERRUPT_PIN2) {
			msa_register_mask_write(handle, NSA_REG_INTERRUPT_MAPPING3, 0x30, 0x30);
      }
      break;
    }
    break;

  case INTERRUPT_OP_DISABLE:
    switch (ops->data.int_src) {
    case INTERRUPT_ACTIVITY:
      /* Enable active interrupt */
      msa_register_mask_write(handle, NSA_REG_INTERRUPT_SETTINGS1, 0x07, 0x00);
      break;

    case INTERRUPT_CLICK:
      /* Enable single and double tap detect */
      msa_register_mask_write(handle, NSA_REG_INTERRUPT_SETTINGS1, 0x30, 0x00);
      break;
    }
    break;

  default:
    MI_ERR("Unsupport operation !");
  }
  return 0;
}

int msa_interrupt_ops(MSA_HANDLE handle, msa_int_ops_t *ops)
{
  int res = 0;

  res = msa_gsensor_drv.obj[gsensor_mod].int_ops(handle, ops);
  return res;
}


int msa_get_enable(MSA_HANDLE handle, char *enable)
{
  int res = 0;
  unsigned char reg_data = 0;

  res = msa_register_read(handle, msa_gsensor_drv.obj[gsensor_mod].power.addr,
							&reg_data);
  if (res != 0) {
    return res;
  }

  *enable = (reg_data & msa_gsensor_drv.obj[gsensor_mod].power.mask) ? 0 : 1;

  return res;
}

int msa_set_enable(MSA_HANDLE handle, char enable)
{
  int res = 0;
  unsigned char reg_data = 0;

  if (!enable) {
    reg_data = msa_gsensor_drv.obj[gsensor_mod].power.value;
  }

  res = msa_register_mask_write(
		handle, msa_gsensor_drv.obj[gsensor_mod].power.addr,
		msa_gsensor_drv.obj[gsensor_mod].power.mask, reg_data);

  return res;
}

static int NSA_get_reg_data(MSA_HANDLE handle, char *buf)
{
  int count = 0;
  int i;
  unsigned char val;

  count +=
      msa_gsensor_drv.method->mysprintf(buf + count, "---------start---------");
  for (i = 0; i <= 0xd2; i++) {
    if (i % 16 == 0)
      count += msa_gsensor_drv.method->mysprintf(buf + count, "\n%02x\t", i);
    msa_register_read(handle, i, &val);
    count += msa_gsensor_drv.method->mysprintf(buf + count, "%02X ", val);
  }

  count += msa_gsensor_drv.method->mysprintf(buf + count,
													"\n--------end---------\n");
  return count;
}

int msa_get_reg_data(MSA_HANDLE handle, char *buf)
{
  return msa_gsensor_drv.obj[gsensor_mod].get_reg_data(handle, buf);
}

int msa_set_odr(MSA_HANDLE handle, int delay)
{
  int res = 0;
  int odr = 0;

  if (delay <= 5) {
    odr = MSA_ODR_200HZ;
  } else if (delay <= 10) {
    odr = MSA_ODR_100HZ;
  } else {
    odr = MSA_ODR_50HZ;
  }

  res = msa_register_mask_write(
		handle, msa_gsensor_drv.obj[gsensor_mod].odr_sect[odr].addr,
		msa_gsensor_drv.obj[gsensor_mod].odr_sect[odr].mask,
		msa_gsensor_drv.obj[gsensor_mod].odr_sect[odr].value);
  if (res != 0) {
    return res;
  }

  return res;
}

static int msa_soft_reset(MSA_HANDLE handle)
{
  int res = 0;
  unsigned char reg_data;

  reg_data = msa_gsensor_drv.obj[gsensor_mod].soft_reset.value;
  res = msa_register_mask_write(
		handle, msa_gsensor_drv.obj[gsensor_mod].soft_reset.addr,
		msa_gsensor_drv.obj[gsensor_mod].soft_reset.mask, reg_data);
  msa_gsensor_drv.method->msdelay(5);

  return res;
}

int msa_module_detect(PLAT_HANDLE handle)
{
  int i, res = 0;
  unsigned char cid, mid;
  int is_find = -1;

  /* Probe gsensor module */
  for (i = 0; i < sizeof(msa_gsensor) / sizeof(msa_gsensor[0]); i++) {
    res = msa_register_read(handle, msa_gsensor[i].chip_id.addr, &cid);
    if (res != 0) {
		return res;
    }

    cid &= msa_gsensor[i].chip_id.mask;
    if (msa_gsensor[i].chip_id.value == cid) {
      res = msa_register_read(handle, msa_gsensor[i].mod_id.addr, &mid);
      if (res != 0) {
		return res;
      }

      mid &= msa_gsensor[i].mod_id.mask;
      if (msa_gsensor[i].mod_id.value == mid) {
		MI_MSG("Found Gsensor MSA !");
		gsensor_mod = i;
		is_find = 0;
		break;
      }
    }
  }
	return is_find;
}

int msa_parse_chip_info(PLAT_HANDLE handle)
{
  unsigned char i = 0, tmp = 0;
  unsigned char reg_value = -1, reg_value1 = -1;
  char res = -1;

  if (-1 == gsensor_mod)
		return res;

  res = msa_register_read(handle, NSA_REG_CHIP_INFO, &reg_value);
  if (res != 0) {
    return res;
  }

  gsensor_chip_info.reg_value = reg_value;

  if (!(reg_value & 0xc0)) {
    gsensor_chip_info.asic = ASIC_2511;
    gsensor_chip_info.mems = MEMS_T9;
    gsensor_chip_info.package = PACKAGE_NONE;

    for (i = 0; i < sizeof(msa_chip_info_list) / sizeof(msa_chip_info_list[0]);
		i++) {
      if (reg_value == msa_chip_info_list[i].reg_value) {
		gsensor_chip_info.package = msa_chip_info_list[i].package;
		gsensor_chip_info.asic = msa_chip_info_list[i].asic;
		gsensor_chip_info.mems = msa_chip_info_list[i].mems;
		break;
      }
    }
  } else {
    gsensor_chip_info.asic = ASIC_2512B;
    gsensor_chip_info.mems = MEMS_T9;
    gsensor_chip_info.package = PACKAGE_NONE;

    gsensor_chip_info.package = (package_type)((reg_value & 0xc0) >> 6);

    if ((reg_value & 0x38) >> 3 == 0x01) {
      gsensor_chip_info.asic = ASIC_2512B;
    }
    res = msa_register_read(handle, NSA_REG_MEMS_OPTION, &reg_value);
    if (res != 0) {
      return res;
    }

    res = msa_register_read(handle, NSA_REG_CHIP_INFO_SECOND, &reg_value1);
    if (res != 0) {
      return res;
    }
    tmp = ((reg_value & 0x01) << 2) | ((reg_value1 & 0xc0) >> 6);

    if (tmp == 0x00) {
      if (reg_value & 0x80)
		gsensor_chip_info.mems = MEMS_TV03;
      else
		gsensor_chip_info.mems = MEMS_T9;
    } else if (tmp == 0x01) {
      gsensor_chip_info.mems = MEMS_RTO3;
    }
  }

  return 0;
}

int msa_install_general_ops(struct general_op_s *ops)
{
  if (0 == ops) {
    return -1;
  }

  msa_gsensor_drv.method = ops;
  return 0;
}

static int msa_check_temp_cali_stable_count = 10;

MSA_HANDLE msa_core_init(PLAT_HANDLE handle)
{
  int res = 0;

  msa_gsensor_drv.obj = msa_gsensor;

  if (gsensor_mod < 0) {
    res = msa_module_detect(handle);
    if (res) {
      MI_ERR("%s: Can't find Msa gsensor!!", __func__);
      return 0;
    }

    /* No msamems gsensor instance found */
    if (gsensor_mod < 0) {
      return 0;
    }
  }

  MI_MSG("Probe gsensor module: %s", msa_gsensor[gsensor_mod].asic);


  res = msa_chip_resume(handle);
  if (res) {
    MI_ERR("chip resume fail!!\n");
    return 0;
  }
    msa_check_temp_cali_stable_count = 10;

  return handle;
}

int msa_chip_resume(MSA_HANDLE handle)
{
  int res = 0;
  unsigned char reg_data;
  unsigned char i = 0;

  res = msa_soft_reset(handle);
  if (res) {
    MI_ERR("Do softreset failed !");
    return res;
  }

  for (i = 0; i < MSA_INIT_SECT_LEN; i++) {
    if (msa_gsensor_drv.obj[gsensor_mod].init_sect[i].addr < 0) {
      break;
    }

    reg_data = msa_gsensor_drv.obj[gsensor_mod].init_sect[i].value;
    res = msa_register_mask_write(
		handle, msa_gsensor_drv.obj[gsensor_mod].init_sect[i].addr,
		msa_gsensor_drv.obj[gsensor_mod].init_sect[i].mask, reg_data);
    if (res != 0) {
      return res;
    }
  }

  msa_gsensor_drv.method->msdelay(10);



  return res;
}

int msa_get_primary_offset(MSA_HANDLE handle, int *x, int *y, int *z)
{
  int res = 0;
  unsigned char reg_data;
  unsigned char i = 0;
  unsigned char offset[9] = {0};

  res = msa_read_offset(handle, offset);
  if (res != 0) {
    MI_ERR("Read offset failed !");
    return -1;
  }

  res = msa_soft_reset(handle);
  if (res) {
    MI_ERR("Do softreset failed !");
    return -1;
  }

  for (i = 0; i < MSA_INIT_SECT_LEN; i++) {
    if (msa_gsensor_drv.obj[gsensor_mod].init_sect[i].addr < 0) {
      break;
    }

    reg_data = msa_gsensor_drv.obj[gsensor_mod].init_sect[i].value;
    res = msa_register_mask_write(
		handle, msa_gsensor_drv.obj[gsensor_mod].init_sect[i].addr,
		msa_gsensor_drv.obj[gsensor_mod].init_sect[i].mask, reg_data);
    if (res != 0) {
      MI_ERR("Write register[0x%x] error!",
		msa_gsensor_drv.obj[gsensor_mod].init_sect[i].addr);
      goto EXIT;
    }
  }

  msa_gsensor_drv.method->msdelay(100);

  res = cycle_read_xyz(handle, x, y, z, 20);
  if (res) {
    MI_ERR("i2c block read failed\n");
    goto EXIT;
  }

  msa_write_offset(handle, offset);

  if ((gsensor_chip_info.reg_value == 0x4B) ||
      (gsensor_chip_info.reg_value == 0x8C) ||
      (gsensor_chip_info.reg_value == 0xCA)) {
    *z = 0;
  }

  return 0;

EXIT:
  msa_write_offset(handle, offset);
  return -1;
}

#define TEMP_CALIBRATE_STATIC_THRESHOLD    120
#define TEMP_CALIBRATE_STATIC_COUNT        12

int msa_temp_calibrate_detect_static(short x, short y, short z)
{
	static int count_static=0;
	static short temp_x[TEMP_CALIBRATE_STATIC_COUNT];
	static short temp_y[TEMP_CALIBRATE_STATIC_COUNT];
	static short temp_z[TEMP_CALIBRATE_STATIC_COUNT];
	static short max_x=0,max_y=0,max_z=0;
	static short min_x=0,min_y=0,min_z=0;
	static char is_first=1;
	int i,delta_sum=0;
	
	count_static++;
	
	if(is_first)    
	{
		temp_x[0] = x;
		temp_y[0] = y;
		temp_z[0] = z;
		for (i=1;i<TEMP_CALIBRATE_STATIC_COUNT;i++)
		{
		temp_x[i] = temp_x[0];
		temp_y[i] = temp_y[0];
		temp_z[i] = temp_z[0];
		}
		is_first = 0;
	}
	else
	{       
		max_x = min_x = temp_x[0];
		max_y = min_y = temp_y[0];
		max_z = min_z = temp_z[0];
		
		for (i=0;i<TEMP_CALIBRATE_STATIC_COUNT-1;i++)
		{
			temp_x[i] = temp_x[i+1];
			temp_y[i] = temp_y[i+1];
			temp_z[i] = temp_z[i+1];
			
			max_x = (max_x > temp_x[i]) ? max_x:temp_x[i];
			max_y = (max_y > temp_y[i]) ? max_y:temp_y[i];
			max_z = (max_z > temp_z[i]) ? max_z:temp_z[i];
			
			min_x = (min_x < temp_x[i]) ? min_x:temp_x[i];
			min_y = (min_y < temp_y[i]) ? min_y:temp_y[i];
			min_z = (min_z < temp_z[i]) ? min_z:temp_z[i];
		}
		temp_x[TEMP_CALIBRATE_STATIC_COUNT-1] = x;
		temp_y[TEMP_CALIBRATE_STATIC_COUNT-1] = y;
		temp_z[TEMP_CALIBRATE_STATIC_COUNT-1] = z;
	}

  if(count_static>TEMP_CALIBRATE_STATIC_COUNT)
  {
    count_static = TEMP_CALIBRATE_STATIC_COUNT;
    delta_sum = abs(max_x - min_x) + abs(max_y - min_y) + abs(max_z - min_z);

    if (delta_sum < TEMP_CALIBRATE_STATIC_THRESHOLD)
      return 1;
  }

  return 0;
}

int msa_temp_calibrate(int *x, int *y, int *z)
{
  int tem_z = 0;
  int cus = MSA_OFFSET_MAX - MSA_OFFSET_CUS;
  int is_static = 0;
  short lz_offset;

  if (*x == 0 && *y == 0 && *z == 0) {
    MI_MSG("invalid data");
    return -1;
  }

  if((x_offset != 0) || (y_offset != 0) || (z_offset != 0)) {
    *x = *x + x_offset;
    *y = *y + y_offset;
    *z = *z + z_offset;
    return 0;
  }

    lz_offset = (*z) % 10;

    MI_DATA("is_static=%d, x= %d, y=%d/r/n", is_static, *x, *y);
    if((abs(*x)<MSA_OFFSET_MAX)&&(abs(*y)<MSA_OFFSET_MAX))
    {
        is_static = msa_temp_calibrate_detect_static(*x,*y,*z-z_offset); 
        tem_z = squareRoot(MSA_OFFSET_SEN*MSA_OFFSET_SEN - (*x)*(*x) - (*y)*(*y)) + lz_offset;
        if(z_offset==0)
        {			  
            if(is_static == 1)
            {
                z_offset = (*z>=0) ? (tem_z-*z) : (-tem_z-*z);
                x_offset = -*x;
                y_offset = -*y;
            }

            *z = ((*z>=0)?(1):(-1))*tem_z;	
        }		
        else if(is_static)
        {
            if(abs(abs(*z)-MSA_OFFSET_SEN)>MSA_OFFSET_CUS)
            {
                *z = ((*z>=0)?(1):(-1))*tem_z;
                z_offset=0;
            }
        }
        *z = ((*z>=0)?(1):(-1))*tem_z;
	}
	else if((abs((abs(*x)-MSA_OFFSET_SEN))<MSA_OFFSET_MAX)&&(abs(*y)<MSA_OFFSET_MAX)&&(z_offset))
	{
		if(abs(*x)>MSA_OFFSET_SEN)
		{
		    *x = (*x>0) ? (*x-(abs(*x)-MSA_OFFSET_SEN)*cus/MSA_OFFSET_MAX):(*x+(abs(*x)-MSA_OFFSET_SEN)*cus/MSA_OFFSET_MAX);
		}
		else
		{
		    *x = (*x>0) ? (*x+(MSA_OFFSET_SEN-abs(*x))*cus/MSA_OFFSET_MAX):(*x-(MSA_OFFSET_SEN-abs(*x))*cus/MSA_OFFSET_MAX);
		}
		*y = (*y)*MSA_OFFSET_CUS/MSA_OFFSET_MAX;
	}
	else if((abs((abs(*y)-MSA_OFFSET_SEN))<MSA_OFFSET_MAX)&&(abs(*x)<MSA_OFFSET_MAX)&&(z_offset))
	{
		if(abs(*y)>MSA_OFFSET_SEN)
		{
	   		*y = (*y>0) ? (*y-(abs(*y)-MSA_OFFSET_SEN)*cus/MSA_OFFSET_MAX):(*y+(abs(*y)-MSA_OFFSET_SEN)*cus/MSA_OFFSET_MAX);
		}
		else
		{
			*y = (*y>0) ? (*y+(MSA_OFFSET_SEN-abs(*y))*cus/MSA_OFFSET_MAX):(*y-(MSA_OFFSET_SEN-abs(*y))*cus/MSA_OFFSET_MAX);
		}
		*x = (*x)*MSA_OFFSET_CUS/MSA_OFFSET_MAX;	
	}
	else if(z_offset==0)
	{
	    if((abs(*x)<MSA_OFFSET_MAX)&&(abs((*y > 0) ? (MSA_OFFSET_SEN-*y):(MSA_OFFSET_SEN+*y))<MSA_OFFSET_MAX))
	    {
			*z = ((*z>=0)?(1):(-1))*abs(*x)*MSA_OFFSET_CUS/MSA_OFFSET_MAX;
	    }
		else if((abs(*y)<MSA_OFFSET_MAX)&&(abs((*x > 0) ? (MSA_OFFSET_SEN-*x):(MSA_OFFSET_SEN+*x))<MSA_OFFSET_MAX))
		{
			*z = ((*z>=0)?(1):(-1))*abs(*y)*MSA_OFFSET_CUS/MSA_OFFSET_MAX;
		}
		else
		{
			tem_z = squareRoot(MSA_OFFSET_SEN*MSA_OFFSET_SEN - (*x)*(*x) - (*y)*(*y)) + lz_offset;
			*z = ((*z>=0)?(1):(-1))*tem_z;
		}
	}	

	MI_MSG("end msa_temp_calibrate x_offset=%d, _offset=%d, z_offset=%d", x_offset, y_offset, z_offset);

	if(z_offset){
		return 0;
	}
	else
		return -1;
}
