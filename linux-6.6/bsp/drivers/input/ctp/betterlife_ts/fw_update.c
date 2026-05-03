#include "bl_ts.h"
#include "bl_fw.h"

/******sktl ctp parameter include files def start******/
//#define BOARD_X_POL
//#define BOARD_Y_POL
//#define SWAP_XY
#if defined(SKTL_AF8638VA)
	#if defined(SKTL_TP_02)
		#include "bl3676_FD080PJ0911A_V10.h"
	#elif defined(SKTL_TP_04)
		#include "bl3676_FD080PJ0911A_V10.h"
	#elif defined(SKTL_TP_05)
		#include "bl3676_FD080PJ0911A_V10.h"
	#else
		#include "bl3676_FD080PJ0911A_V10.h"
	#endif
#elif defined(SKTL_AF8638VAT)
	#if defined(SKTL_TP_06)
		#include "bl3676_FD080PJ0911A_V10.h"
	#elif defined(SKTL_TP_09)
		#include "bl3676_FD080PJ0911A_V10.h"
	#else
		#include "bl3676_FD080PJ0911A_V10.h"
	#endif
#elif defined(SKTL_AF86310VC)
	#if defined(SKTL_TP_03)
		#include "bl3676_FD080PJ0911A_V10.h"
	#else
		#include "bl3676_FD080PJ0911A_V10.h"
	#endif

	#ifdef CONFIG_AW_DRAGONBOARD_TEST
		#undef	SCREEN_MAX_X
		#undef	SCREEN_MAX_Y
		#define BOARD_Y_POL
		#define SWAP_XY
		#define SCREEN_MAX_X 1280
		#define SCREEN_MAX_Y 800
	#endif

#elif defined(SKTL_AF86310VAT)
	#if defined(SKTL_TP_07)
		#include "bl3676_PX101G24A021_V1_0.h"
	#else
		#include "bl3676_FD080PJ0911A_V10.h"
	#endif

#else
	#include "bl3676_FD080PJ0911A_V10.h"
#endif

/******sktl ctp parameter include files def ends******/

static int btl_i2c_transfer(unsigned char i2c_addr, unsigned char *buf, int len,unsigned char rw)
{
	int ret;
    switch(rw)
    {
        case I2C_WRITE:
			ret = btl_i2c_write(g_btl_ts->client, i2c_addr, buf, len);
			break;
		case I2C_READ:
			ret = btl_i2c_read(g_btl_ts->client, i2c_addr, buf, len);
			break;			
    }
	if(ret < 0){
		BTL_DEBUG("btl_i2c_transfer:i2c transfer error___\n");
		return -1;
	}

	return 0;
}

static int btl_read_fw(unsigned char i2c_addr,unsigned char reg_addr, unsigned char *buf, int len)
{
	int ret;

	ret = btl_i2c_write(g_btl_ts->client, i2c_addr, &reg_addr, 1);
	if(ret < 0)
	{
		goto IIC_COMM_ERROR;
	}
	ret = btl_i2c_read(g_btl_ts->client, i2c_addr, buf, len);
	if(ret < 0)
	{
		goto IIC_COMM_ERROR;
	}	

IIC_COMM_ERROR: 
	if(ret < 0){
		BTL_DEBUG("btl_read_fw:i2c transfer error___\n");
		return -1;
	}

	return 0;
}

int btl_soft_reset_switch_int_wakemode(void)
{
    unsigned char cmd[4];
	int ret = 0x00;

	cmd[0] = RW_REGISTER_CMD;
	cmd[1] = ~cmd[0];
	cmd[2] = CHIP_ID_REG;
    cmd[3] = 0xe8;
	
	ret = btl_i2c_transfer(CTP_SLAVE_ADDR, cmd,4,I2C_WRITE);
	if(ret < 0){
		BTL_DEBUG("btl_soft_reset_switch_int_wakemode failed:i2c write flash error___\n");
	}

    return ret;
}

int btl_get_chip_id(unsigned char *buf)
{

	unsigned char cmd[3];
	int ret = 0x00;
	unsigned char retry = 3;
	BTL_DEBUG("btl_get_chip_id\n");

	while(retry--)
    {    
    	cmd[0] = RW_REGISTER_CMD;
    	cmd[1] = ~cmd[0];
    	cmd[2] = CHIP_ID_REG;
    
	    SET_WAKEUP_LOW;
	    btl_i2c_lock();
    	ret = btl_i2c_transfer(BTL_FLASH_I2C_ADDR, cmd,3,I2C_WRITE);
    	if(ret < 0){
    		BTL_DEBUG("btl_get_chip_id:i2c write flash error___\n");
    		goto GET_CHIP_ID_ERROR;
    	}
    	
    	ret = btl_i2c_transfer(BTL_FLASH_I2C_ADDR, buf,1,I2C_READ); 
    	if(ret < 0){
    		BTL_DEBUG("btl_get_chip_id:i2c read flash error___\n");
    		goto GET_CHIP_ID_ERROR;
    	}

        BTL_DEBUG("btl_get_chip_id:buf = %x\n",*buf);
		if(*buf == BTL_FLASH_ID)
        {
            btl_i2c_unlock();	
		    SET_WAKEUP_HIGH;
            break;
        }
		
		GET_CHIP_ID_ERROR:
			btl_i2c_unlock();	
		    SET_WAKEUP_HIGH;
            continue;
    }

	return ret;
}

static void btl_switch_protocol(void)
{
	int ret;
    unsigned char cmd[] = {'U', 'F', 'O'};	
	ret = btl_i2c_transfer(CTP_SLAVE_ADDR, cmd,sizeof(cmd),I2C_WRITE);
	if(ret < 0){
		BTL_DEBUG("failed\n");
	}	
	MDELAY(30);	
}

#if(UPDATE_MODE == I2C_UPDATE_MODE_NEW)
void btl_enter_update_with_i2c(void)
{
    unsigned char buf[4] = {0x63, 0x75, 0x69, 0x33};
	unsigned char cmd[16] = {0};
	int i = 4;
	int ret = 0;

	for(i = 0; i < 4; i++) 
    {
        cmd[4 * i] = buf[0];
		cmd[4 * i + 1] = buf[1];
		cmd[4 * i + 2] = buf[2];
		cmd[4 * i + 3] = buf[3];
    }

	ret = btl_i2c_transfer(CTP_SLAVE_ADDR, cmd, sizeof(cmd), I2C_WRITE);
	if(ret < 0)
    {
        BTL_DEBUG("bl_enter_update_with_i2c failed:send i2c cmd error___\r\n");
        goto error;
    }
	MDELAY(50);

error:
	return ;
}
#endif

#if(UPDATE_MODE == I2C_UPDATE_MODE_OLD)
void btl_enter_update_with_i2c(void)
{
    unsigned char cmd[200] = {0x00};
	int i = 0;
	int ret = 0;

	for(i = 0; i < sizeof(cmd); i += 2)
    {
        cmd[i] = 0x5a;
		cmd[i + 1] = 0xa5;
    }

	ret = btl_i2c_transfer(CTP_SLAVE_ADDR, cmd, sizeof(cmd), I2C_WRITE);
	if(ret < 0)
    {
        BTL_DEBUG("btl_enter_update_with_i2c failed:send 5a a5 error___\n");
        goto error;
    }
	MDELAY(50);

error:
	return ;
}
#endif

void btl_exit_update_with_i2c(void)
{
    int ret = 0;
	unsigned char cmd[2] = {0x5a, 0xa5};
    MDELAY(20);
	ret = btl_i2c_transfer(CTP_SLAVE_ADDR, cmd, sizeof(cmd), I2C_WRITE);
	if(ret < 0)
    {
        BTL_DEBUG("btl_exit_update_with_i2c failed:send 5a a5 error___\n");
    }

	MDELAY(20);
	#if defined(RESET_PIN_WAKEUP)
	btl_ts_reset_wakeup();
	#endif
	MDELAY(30);
	btl_switch_protocol();
	return;
}

#if(UPDATE_MODE == INT_UPDATE_MODE)
void btl_enter_update_with_int(void)
{
    btl_ts_set_intmode(0);
    btl_ts_set_intup(0);
	#if defined(RESET_PIN_WAKEUP)
	btl_ts_reset_wakeup();
	#endif
    btl_soft_reset_switch_int_wakemode();
	MDELAY(50);
}

void btl_exit_update_with_int(void)
{
    MDELAY(20);
	btl_ts_set_intup(1);
	MDELAY(20);
	btl_ts_set_intmode(1);
    #if defined(RESET_PIN_WAKEUP)
	btl_ts_reset_wakeup();
    #endif
	MDELAY(30);
	btl_switch_protocol();
}
#endif

int btl_get_fwArgPrj_id(unsigned char *buf)
{
	BTL_DEBUG("btl_get_fwArgPrj_id\n");
	return btl_read_fw(CTP_SLAVE_ADDR,BTL_FWVER_PJ_ID_REG, buf, 3);
}

int btl_get_prj_id(unsigned char *buf)
{
	BTL_DEBUG("btl_get_prj_id\n");
	return btl_read_fw(CTP_SLAVE_ADDR,BTL_PRJ_ID_REG, buf, 1);    
}

int btl_get_cob_id(unsigned char *buf)
{
	BTL_DEBUG("btl_get_cob_id\n");
	return btl_read_fw(CTP_SLAVE_ADDR,COB_ID_REG, buf, 6);
}

#ifdef BTL_UPDATE_FIRMWARE_ENABLE
static int btl_get_protect_flag(void)
{
    unsigned char ret = 0;
    unsigned char protectFlag = 0x00;
	BTL_DEBUG("btl_get_protect_flag\n");
	ret = btl_read_fw(CTP_SLAVE_ADDR,BTL_PROTECT_REG, &protectFlag, 1);
	if(ret < 0)
    {
    	BTL_DEBUG("btl_get_protect_flag failed,ret = %x\n",ret);
		return 0;
    }
	if(protectFlag == 0x55)
    {
        BTL_DEBUG("btl_get_protect_flag:protectFlag = %x\n",protectFlag);
		return 1;
    }
	return 0;
}

static int btl_get_specific_argument_for_self_ctp(unsigned int *arguOffset, unsigned char *cobID, unsigned char* fw_data, unsigned int fw_size, unsigned char arguCount)
{
    unsigned char convertCobId[12] = {0x00};
    unsigned char i = 0;
    unsigned int cobArguAddr = fw_size - arguCount * BTL_ARGUMENT_FLASH_SIZE;
    BTL_DEBUG("fw_size is %x\n", fw_size);
    BTL_DEBUG("arguCount is %d\n", arguCount);
    BTL_DEBUG("cobArguAddr is %x\n",cobArguAddr);
	
    for(i = 0; i < sizeof(convertCobId); i++)
    {		
        if(i%2)
        {
            convertCobId[i] = cobID[i/2] & 0x0f;
        }		
        else
        {
            convertCobId[i] = (cobID[i/2] & 0xf0) >> 4;
        }
        BTL_DEBUG("before convert:convertCobId[%d] is %x\n",i,convertCobId[i]);
        if(convertCobId[i] < 10)
        {
            convertCobId[i] = '0' + convertCobId[i];
        }
        else
        {
            convertCobId[i] = 'a' + convertCobId[i] - 10;
        }
        BTL_DEBUG("after convert:convertCobId[%d] is %x\n",i,convertCobId[i]);
    }
	
    BTL_DEBUG("convertCobId is:\n");
    for(i= 0; i < 12; i++)
    {
        BTL_DEBUG("%x  ", convertCobId[i]);
    }
    BTL_DEBUG("\n");
	
    for(i = 0; i < arguCount; i++)
    {
        if(memcmp(convertCobId, fw_data + cobArguAddr + i * BTL_ARGUMENT_FLASH_SIZE + BTL_COB_ID_OFFSET, 12))
        {
            BTL_DEBUG("This argu is not the specific argu\n");
        }
        else
        {
            *arguOffset = cobArguAddr + i * BTL_ARGUMENT_FLASH_SIZE;
            BTL_DEBUG("This argu is the specific argu, and arguOffset is %x\n",*arguOffset);
            break;
        }
    }

	if(i == arguCount)
    {
        *arguOffset = BTL_ARGUMENT_BASE_OFFSET;
        return -1;
    }
	else
    {
        return 0;
    }
}

static int btl_get_specific_argument_for_self_interactive_ctp(unsigned int *arguOffset, unsigned char prjID, unsigned char* fw_data, unsigned int fw_size, unsigned char arguCount)
{
    int i = 0;
	unsigned char binPrjID = 0x00;

    BTL_DEBUG_FUNC();
	BTL_DEBUG("prjID = %d, fw_size = %x, arguCount = %d\n", prjID, fw_size, arguCount);
	for(i = 0; i < arguCount; i++)
    {
        binPrjID = fw_data[BTL_ARGUMENT_BASE_OFFSET + i * MAX_FLASH_SIZE + BTL_PROJECT_ID_OFFSET];
		BTL_DEBUG("i = %d, binPrjID = %d\n", i, binPrjID);
        if(prjID == binPrjID)
        {
            *arguOffset = i * MAX_FLASH_SIZE;
            break;
        }
        else
        {
            continue;
        }
    }

    if(i >= arguCount)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}


static unsigned char btl_get_argument_count_for_self_ctp(unsigned char* fw_data, unsigned int fw_size)
{
    unsigned char i = 0;
	unsigned addr = 0;
	addr = fw_size;
	BTL_DEBUG("addr is %x\n", addr);
	while(addr > (BTL_ARGUMENT_BASE_OFFSET + BTL_ARGUMENT_FLASH_SIZE))
    {
        addr = addr - BTL_ARGUMENT_FLASH_SIZE;
        if(memcmp(fw_data+addr, ARGU_MARK, sizeof(ARGU_MARK) - 1))
        {
            BTL_DEBUG("arguMark found flow complete");
            break;		
        }
        else
        {
            i++;
            BTL_DEBUG("arguMark founded\n");
        }
    }
    BTL_DEBUG("The argument count is %d\n",i);
    return i;
}

static unsigned int btl_get_cob_project_down_size_arguCnt_for_self_ctp(unsigned char* fw_data,unsigned int fw_size, unsigned char *arguCnt)
{
    unsigned int downSize = 0;

	*arguCnt = btl_get_argument_count_for_self_ctp(fw_data,fw_size);
	downSize = fw_size - (*arguCnt) * BTL_ARGUMENT_FLASH_SIZE - FLASH_PAGE_SIZE;
	return downSize;
}

static unsigned char btl_get_argument_count_for_self_interactive_ctp(unsigned char* fw_data, unsigned int fw_size)
{
    unsigned char i = 0;
    i = fw_size / MAX_FLASH_SIZE;
    BTL_DEBUG("The argument count is %d\n",i);
    return i;
}

static unsigned int btl_get_cob_project_down_size_arguCnt_for_interactive_ctp(unsigned char* fw_data,unsigned int fw_size, unsigned char *arguCnt)
{
    unsigned int downSize = 0;
    *arguCnt = btl_get_argument_count_for_self_interactive_ctp(fw_data,fw_size);
	downSize = MAX_FLASH_SIZE;
	return downSize;
}

static unsigned char btl_is_cob_project_for_self(unsigned char* fw_data, int fw_size)
{
    unsigned char arguKey[4] = {0xaa,0x55,0x09,0x09};
	unsigned char* pfw;

	pfw = fw_data+fw_size-4;
	
    if(fw_size%FLASH_PAGE_SIZE)
    {
        return 0;
    }
	else
    {
	    if(memcmp(arguKey,pfw,4))
	    {
	        return 0;
	    }
        else
        {
            return 1;
        }
    }
}

static unsigned char btl_is_cob_project_for_self_interactive(unsigned char* fw_data, int fw_size)
{
    if((fw_size > MAX_FLASH_SIZE) && ((fw_size % MAX_FLASH_SIZE)==0))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

static int btl_get_fw_checksum(unsigned short *fw_checksum)
{
	unsigned char buf[3];
	unsigned char checksum_ready = 0;
	int retry = 5;
	int ret = 0x00;

	BTL_DEBUG("btl_get_fw_checksum\n");

	buf[0] = CHECKSUM_CAL_REG;
	buf[1] = CHECKSUM_CAL;
	ret = btl_i2c_transfer(CTP_SLAVE_ADDR, buf,2,I2C_WRITE);
	if(ret < 0){
		BTL_DEBUG("btl_get_fw_checksum:write checksum cmd error___\n");
		return -1;
	}
	MDELAY(FW_CHECKSUM_DELAY_TIME);
	
	ret = btl_read_fw(CTP_SLAVE_ADDR,CHECKSUM_REG, buf, 3);
	if(ret < 0){
		BTL_DEBUG("btl_get_fw_checksum:read checksum error___\n");
		return -1;
	}

	checksum_ready = buf[0];

	while((retry--) && (checksum_ready != CHECKSUM_READY)){

		MDELAY(50);
		ret = btl_read_fw(CTP_SLAVE_ADDR,CHECKSUM_REG, buf, 3);
		if(ret < 0){
			BTL_DEBUG("btl_get_fw_checksum:read checksum error___\n");
			return -1;
		}

		checksum_ready = buf[0];
	}
	
	if(checksum_ready != CHECKSUM_READY){
		BTL_DEBUG("btl_get_fw_checksum:read checksum fail___\n");
		return -1;
	}
	*fw_checksum = (buf[1]<<8)+buf[2];

	return 0;
}

static void btl_get_fw_bin_checksum_for_self_ctp(unsigned char *fw_data,unsigned short *fw_bin_checksum, int fw_size, int specifyArgAddr)
{
	int i = 0;
	int temp_checksum = 0x0;

    for(i = 0; i < BTL_ARGUMENT_BASE_OFFSET; i++)
    {
        temp_checksum += fw_data[i];
    }
    for(i = specifyArgAddr; i < specifyArgAddr + VERTIFY_START_OFFSET; i++)
    {
    	temp_checksum += fw_data[i];
    }
    for(i = specifyArgAddr + VERTIFY_START_OFFSET; i < specifyArgAddr + VERTIFY_START_OFFSET + 4; i++)
    {
    	temp_checksum += fw_data[i];
    }
    for(i = BTL_ARGUMENT_BASE_OFFSET + VERTIFY_START_OFFSET + 4; i < fw_size; i++)
    {
    	temp_checksum += fw_data[i];
    }

    for(i = fw_size; i < MAX_FLASH_SIZE; i++)
    {
    	temp_checksum += 0xff;
    }
    
    *fw_bin_checksum = temp_checksum & 0xffff;
}

static void btl_get_fw_bin_checksum_for_self_interactive_ctp(unsigned char *fw_data,unsigned short *fw_bin_checksum, int fw_size, int specifyArgAddr)
{
	int i = 0;
	int temp_checksum = 0x0;

    for(i = specifyArgAddr; i < (specifyArgAddr + fw_size); i++)
    {
    	temp_checksum += fw_data[i];
    }    

	for(i = fw_size; i < MAX_FLASH_SIZE; i++)
    {
        temp_checksum += 0xff;
    }
    *fw_bin_checksum = temp_checksum & 0xffff;
}

static void btl_get_fw_bin_checksum_for_compatible_ctp(unsigned char *fw_data,unsigned short *fw_bin_checksum, int fw_size)
{
	int i = 0;
	int temp_checksum = 0x0;

    for(i = 0; i < fw_size; i++)
    {
    	temp_checksum += fw_data[i];
    }
    for(i = fw_size; i < MAX_FLASH_SIZE; i++)
    {
    	temp_checksum += 0xff;
    }
    
    *fw_bin_checksum = temp_checksum & 0xffff;
}

static int btl_erase_flash(void)
{
	unsigned char cmd[2];
	
	BTL_DEBUG("btl_erase_flash\n");
	
	cmd[0] = ERASE_ALL_MAIN_CMD; 
	cmd[1] = ~cmd[0];

	return btl_i2c_transfer(BTL_FLASH_I2C_ADDR,cmd, 0x02,I2C_WRITE);	
}

__attribute__((unused)) static int btl_write_flash_no_blm18(unsigned char cmd, int flash_start_addr, unsigned char *buf, int len)
{
	unsigned char cmd_buf[6+FLASH_WSIZE];
	unsigned int flash_end_addr;
	int ret;
		
	//BTL_DEBUG("btl_write_flash_no_blm18\n");
	//BTL_DEBUG("flash_start_addr = %x:%x %x len = %x\n", flash_start_addr, buf[0], buf[1], len);
	if(!len){
		BTL_DEBUG("___write flash len is 0x00,return___\n");
		return -1;	
	}

	flash_end_addr = flash_start_addr + len -1;

	if(flash_end_addr >= MAX_FLASH_SIZE){
		BTL_DEBUG("___write flash end addr is overflow,return___\n");
		return -1;	
	}

	cmd_buf[0] = cmd;
	cmd_buf[1] = ~cmd;
	cmd_buf[2] = flash_start_addr >> 0x08;
	cmd_buf[3] = flash_start_addr & 0xff;
	cmd_buf[4] = flash_end_addr >> 0x08;
	cmd_buf[5] = flash_end_addr & 0xff;

	memcpy(&cmd_buf[6],buf,len);

	ret = btl_i2c_transfer(BTL_FLASH_I2C_ADDR,cmd_buf, len+6,I2C_WRITE);	
	if(ret < 0){
		BTL_DEBUG("i2c transfer error___\n");
		return -1;
	}

	return 0;
}

__attribute__((unused)) static int btl_write_flash_blm18(unsigned char cmd, int flash_start_addr, unsigned char *buf, int len)
{
	unsigned char cmd_buf[8+FLASH_WSIZE];
	unsigned int flash_end_addr;
	int ret;

	BTL_DEBUG("btl_write_flash_blm18\n");
	
	if(!len){
		BTL_DEBUG("___write flash len is 0x00,return___\n");
		return -1;	
	}

	flash_end_addr = flash_start_addr + len -1;

	if(flash_end_addr >= MAX_FLASH_SIZE){
		BTL_DEBUG("___write flash end addr is overflow,return___\n");
		return -1;	
	}

	cmd_buf[0] = cmd;
	cmd_buf[1] = ~cmd;
	cmd_buf[2] = flash_start_addr >> 16;
	cmd_buf[3] = flash_start_addr >> 8;
	cmd_buf[4] = flash_start_addr & 0xff;
	cmd_buf[5] = flash_end_addr >> 16;
	cmd_buf[6] = flash_end_addr >> 8;
	cmd_buf[7] = flash_end_addr & 0xff;

	memcpy(&cmd_buf[8],buf,len);

	ret = btl_i2c_transfer(BTL_FLASH_I2C_ADDR,cmd_buf, len+8,I2C_WRITE);	
	if(ret < 0){
		BTL_DEBUG("i2c transfer error___\n");
		return -1;
	}

	return 0;
}

static int btl_write_flash(unsigned char cmd, int flash_start_addr, unsigned char *buf, int len)
{
    #if(TS_CHIP==BLM18)
	    return btl_write_flash_blm18(cmd, flash_start_addr, buf, len);
    #else
	    return btl_write_flash_no_blm18(cmd, flash_start_addr, buf, len);
	#endif
}

__attribute__((unused)) static int btl_read_flash_no_blm18(unsigned char cmd, int flash_start_addr, unsigned char *buf, int len)
{
	char ret =0;
	unsigned char cmd_buf[6];
	unsigned int flash_end_addr;

	flash_end_addr = flash_start_addr + len -1;
	cmd_buf[0] = cmd;
	cmd_buf[1] = ~cmd;
	cmd_buf[2] = flash_start_addr >> 0x08;
	cmd_buf[3] = flash_start_addr & 0xff;
	cmd_buf[4] = flash_end_addr >> 0x08;
	cmd_buf[5] = flash_end_addr & 0xff;
	ret = btl_i2c_transfer(BTL_FLASH_I2C_ADDR,cmd_buf,sizeof(cmd_buf),I2C_WRITE);
	if(ret < 0)
	{
	    BTL_DEBUG("btl_read_flash_no_blm18:i2c transfer write error\n");
		return -1;
	}
	ret = btl_i2c_transfer(BTL_FLASH_I2C_ADDR,buf,len,I2C_READ);
	if(ret < 0)
	{
	    BTL_DEBUG("btl_read_flash_no_blm18:i2c transfer read error\n");
		return -1;
	}

	return 0;
}

__attribute__((unused)) static int btl_read_flash_blm18(unsigned char cmd, int flash_start_addr, unsigned char *buf, int len)
{
	char ret =0;
	unsigned char cmd_buf[8];
	unsigned int flash_end_addr;

	flash_end_addr = flash_start_addr + len -1;
	cmd_buf[0] = cmd;
	cmd_buf[1] = ~cmd;
	cmd_buf[2] = flash_start_addr >> 16;
	cmd_buf[3] = flash_start_addr >> 8;
	cmd_buf[4] = flash_start_addr & 0xff;
	cmd_buf[5] = flash_end_addr >> 16;
	cmd_buf[6] = flash_end_addr >> 8;
	cmd_buf[7] = flash_end_addr & 0xff;
	ret = btl_i2c_transfer(BTL_FLASH_I2C_ADDR,cmd_buf,sizeof(cmd_buf),I2C_WRITE);
	if(ret < 0)
	{
	    BTL_DEBUG("btl_read_flash_blm18:i2c transfer write error\n");
		return -1;
	}
	ret = btl_i2c_transfer(BTL_FLASH_I2C_ADDR,buf,len,I2C_READ);
	if(ret < 0)
	{
	    BTL_DEBUG("btl_read_flash_blm18:i2c transfer read error\n");
		return -1;
	}

	return 0;
}

static int btl_read_flash(unsigned char cmd, int flash_start_addr, unsigned char *buf, int len)
{
    #if(TS_CHIP==BLM18)
        return btl_read_flash_blm18(cmd, flash_start_addr, buf, len);
    #else
        return btl_read_flash_no_blm18(cmd, flash_start_addr, buf, len);
    #endif
}

static int btl_download_fw_for_self_ctp(unsigned char *pfwbin,int specificArgAddr, int fwsize)
{
	unsigned int i;
	unsigned int size,len;
	unsigned int addr;
    unsigned char verifyBuf[4] = {0xff, 0xff, 0xff, 0xff};
	BTL_DEBUG("btl_download_fw_for_self_ctp\n");
	
	verifyBuf[2] = pfwbin[BTL_ARGUMENT_BASE_OFFSET+VERTIFY_START_OFFSET+2];
	verifyBuf[3] = pfwbin[BTL_ARGUMENT_BASE_OFFSET+VERTIFY_START_OFFSET+3];	
	BTL_DEBUG("btl_download_fw:verifyBuf = %x %x %x %x\n",verifyBuf[0],verifyBuf[1],verifyBuf[2],verifyBuf[3]);
	if(btl_erase_flash()){
		BTL_DEBUG("___erase flash fail___\n");
		return -1;
	}

	MDELAY(50);

    //Write data before BTL_ARGUMENT_BASE_OFFSET
	for(i=0;i< BTL_ARGUMENT_BASE_OFFSET;)
	{
		size = BTL_ARGUMENT_BASE_OFFSET - i;
		if(size > FLASH_WSIZE){
			len = FLASH_WSIZE;
		}else{
			len = size;
		}

		addr = i;
	
		if(btl_write_flash(WRITE_MAIN_CMD,addr, &pfwbin[i],len)){
			return -1;
		}
		i += len;
		MDELAY(5);
	}

    //Write the data from BTL_ARGUMENT_BASE_OFFSET to VERTIFY_START_OFFSET
    for(i=BTL_ARGUMENT_BASE_OFFSET;i< (VERTIFY_START_OFFSET+BTL_ARGUMENT_BASE_OFFSET);)
    {
    	size = VERTIFY_START_OFFSET + BTL_ARGUMENT_BASE_OFFSET - i;
    	if(size > FLASH_WSIZE){
    		len = FLASH_WSIZE;
    	}else{
    		len = size;
    	}
    
    	addr = i;
    
    	if(btl_write_flash(WRITE_MAIN_CMD,addr, &pfwbin[i+specificArgAddr-BTL_ARGUMENT_BASE_OFFSET],len)){
    		return -1;
    	}
    	i += len;
    	MDELAY(5);
    }

    //Write the four bytes verifyBuf from VERTIFY_START_OFFSET
    for(i=(VERTIFY_START_OFFSET + BTL_ARGUMENT_BASE_OFFSET);i< (VERTIFY_START_OFFSET + BTL_ARGUMENT_BASE_OFFSET + sizeof(verifyBuf));)
    {
    	size = VERTIFY_START_OFFSET + BTL_ARGUMENT_BASE_OFFSET + sizeof(verifyBuf) - i;
    	if(size > FLASH_WSIZE){
    		len = FLASH_WSIZE;
    	}else{
    		len = size;
    	}
    
    	addr = i;
    
    	if(btl_write_flash(WRITE_MAIN_CMD,addr, &verifyBuf[i-VERTIFY_START_OFFSET-BTL_ARGUMENT_BASE_OFFSET],len)){
    		return -1;
    	}
    	i += len;
    	MDELAY(5);
    }

    //Write data after verityBuf from VERTIFY_START_OFFSET + 4
    for(i=(BTL_ARGUMENT_BASE_OFFSET + VERTIFY_START_OFFSET + 4);i< fwsize;)
    {
    	size = fwsize - i;
    	if(size > FLASH_WSIZE){
    		len = FLASH_WSIZE;
    	}else{
    		len = size;
    	}
    
    	addr = i;
    
    	if(btl_write_flash(WRITE_MAIN_CMD,addr, &pfwbin[i],len)){
    		return -1;
    	}
    	i += len;
    	MDELAY(5);
    }

	return 0;	
}

static int btl_download_fw_for_self_interactive_ctp(unsigned char *pfwbin,int specificArgAddr, int fwsize)
{
	unsigned int i;
	unsigned int size,len;
	unsigned int addr;
    unsigned char verifyBuf[4] = {0xff, 0xff, 0xff, 0xff};
	BTL_DEBUG("btl_download_fw_for_self_interactive_ctp\n");
	
	verifyBuf[2] = pfwbin[specificArgAddr + BTL_ARGUMENT_BASE_OFFSET+VERTIFY_START_OFFSET+2];
	verifyBuf[3] = pfwbin[specificArgAddr + BTL_ARGUMENT_BASE_OFFSET+VERTIFY_START_OFFSET+3];	
	BTL_DEBUG("btl_download_fw_for_self_ctp:verifyBuf = %x %x %x %x\n",verifyBuf[0],verifyBuf[1],verifyBuf[2],verifyBuf[3]);
	if(btl_erase_flash()){
		BTL_DEBUG("___erase flash fail___\n");
		return -1;
	}

	MDELAY(50);

    //Write data before BTL_ARGUMENT_BASE_OFFSET
	for(i=0;i< (VERTIFY_START_OFFSET+BTL_ARGUMENT_BASE_OFFSET);)
	{
		size = BTL_ARGUMENT_BASE_OFFSET + VERTIFY_START_OFFSET - i;
		if(size > FLASH_WSIZE){
			len = FLASH_WSIZE;
		}else{
			len = size;
		}

		addr = i;
	
		if(btl_write_flash(WRITE_MAIN_CMD,addr, &pfwbin[i+specificArgAddr],len)){
			return -1;
		}
		i += len;
		MDELAY(5);
	}

    //Write the four bytes verifyBuf from VERTIFY_START_OFFSET
    for(i=(VERTIFY_START_OFFSET + BTL_ARGUMENT_BASE_OFFSET);i< (VERTIFY_START_OFFSET + BTL_ARGUMENT_BASE_OFFSET + sizeof(verifyBuf));)
    {
    	size = VERTIFY_START_OFFSET + BTL_ARGUMENT_BASE_OFFSET + sizeof(verifyBuf) - i;
    	if(size > FLASH_WSIZE){
    		len = FLASH_WSIZE;
    	}else{
    		len = size;
    	}
    
    	addr = i;
    
    	if(btl_write_flash(WRITE_MAIN_CMD,addr, &verifyBuf[i-VERTIFY_START_OFFSET-BTL_ARGUMENT_BASE_OFFSET],len)){
    		return -1;
    	}
    	i += len;
    	MDELAY(5);
    }

    //Write data after verityBuf from VERTIFY_START_OFFSET + 4
    for(i=(BTL_ARGUMENT_BASE_OFFSET + VERTIFY_START_OFFSET + 4);i < fwsize;)
    {
    	size = fwsize - i;
    	if(size > FLASH_WSIZE){
    		len = FLASH_WSIZE;
    	}else{
    		len = size;
    	}
    
    	addr = i;
    
    	if(btl_write_flash(WRITE_MAIN_CMD,addr, &pfwbin[i+specificArgAddr],len)){
    		return -1;
    	}
    	i += len;
    	MDELAY(5);
    }

	return 0;	
}

static int btl_download_fw_for_compatible_ctp(unsigned char *pfwbin, int fwsize)
{
	unsigned int i;
	unsigned int size,len;
	unsigned int addr;
	BTL_DEBUG("btl_download_fw_for_compatible_ctp\n");
	
	if(btl_erase_flash()){
		BTL_DEBUG("___erase flash fail___\n");
		return -1;
	}

	MDELAY(50);

    for(i = 0;i < fwsize;)
    {
    	size = fwsize - i;
    	if(size > FLASH_WSIZE){
    		len = FLASH_WSIZE;
    	}else{
    		len = size;
    	}
    
    	addr = i;
    
    	if(btl_write_flash(WRITE_MAIN_CMD,addr, &pfwbin[i],len)){
    		return -1;
    	}
    	i += len;
    	MDELAY(5);
    }

	return 0;	
}

static int btl_read_flash_vertify(unsigned char *pfwbin)
{
	unsigned char cnt = 0;
	int ret = 0;
	unsigned char vertify[2] = {0};
	unsigned char vertify1[2] = {0};
	
	memcpy(vertify,&pfwbin[BTL_ARGUMENT_BASE_OFFSET + VERTIFY_START_OFFSET],sizeof(vertify));
	BTL_DEBUG("btl_read_flash_vertify: vertify:%x %x\n",vertify[0],vertify[1]);

	SET_WAKEUP_LOW;
	while(cnt < 3)
	{
	    cnt++;
        ret = btl_read_flash(READ_MAIN_CMD, BTL_ARGUMENT_BASE_OFFSET + VERTIFY_START_OFFSET, vertify1, sizeof(vertify1));
		if(ret < 0)
		{
			BTL_DEBUG("btl_write_flash_vertify: read fail\n");
			continue;
		}

		if(memcmp(vertify, vertify1, sizeof(vertify)) == 0)
		{
			ret = 0;
			break;
		}
		else
        {
            ret = -1;
        }
	}
	SET_WAKEUP_HIGH;
	return ret;
}

static int btl_write_flash_vertify(unsigned char *pfwbin)
{
	unsigned char cnt = 0;
	int ret = 0;
	unsigned char vertify[2] = {0};
	unsigned char vertify1[2] = {0};

	memcpy(vertify,&pfwbin[BTL_ARGUMENT_BASE_OFFSET + VERTIFY_START_OFFSET],sizeof(vertify));
	BTL_DEBUG("btl_write_flash_vertify: vertify:%x %x\n",vertify[0],vertify[1]);

	SET_WAKEUP_LOW;
	while(cnt < 3)
	{
	    cnt++;
	    ret = btl_write_flash(WRITE_MAIN_CMD, BTL_ARGUMENT_BASE_OFFSET + VERTIFY_START_OFFSET, vertify, sizeof(vertify));
		if(ret < 0)
		{
			BTL_DEBUG("btl_write_flash_vertify: write fail\n");
			continue;
		}
		
		MDELAY(10);

        ret = btl_read_flash(READ_MAIN_CMD, BTL_ARGUMENT_BASE_OFFSET + VERTIFY_START_OFFSET, vertify1, sizeof(vertify1));
		if(ret < 0)
		{
			BTL_DEBUG("btl_write_flash_vertify: read fail\n");
			continue;
		}

		if(memcmp(vertify, vertify1, sizeof(vertify)) == 0)
		{
			ret = 0;
			break;
		}
		else
        {
            ret = -1;
        }
	}
	SET_WAKEUP_HIGH;
	return ret;
}

static int btl_update_flash_for_self_ctp(unsigned char update_type, unsigned char *pfwbin,int fwsize, int specificArgAddr)
{
	int retry = 0;
	int ret = 0;
	unsigned short fw_checksum = 0x0;
	unsigned short fw_bin_checksum = 0x0;
	retry =3;
	while(retry--)
	{
		SET_WAKEUP_LOW;

	    ret = btl_download_fw_for_self_ctp(pfwbin,specificArgAddr,fwsize);

		if(ret<0)
		{
			BTL_DEBUG("btl_update_flash_for_self_ctp:btl_download_fw_for_self_ctp error retry=%d\n",retry);
			continue;
		}
		
		MDELAY(50);

		SET_WAKEUP_HIGH;
	
	    btl_get_fw_bin_checksum_for_self_ctp(pfwbin,&fw_bin_checksum, fwsize,specificArgAddr);
	    ret = btl_get_fw_checksum(&fw_checksum);
		fw_checksum -= 0xff;
		BTL_DEBUG("btl_download_fw_for_self_ctp:fw checksum = 0x%x,fw_bin_checksum =0x%x\n",fw_checksum, fw_bin_checksum);

		
		if((ret < 0) || ((update_type == FW_ARG_UPDATE)&&(fw_checksum != fw_bin_checksum)))
		{
			BTL_DEBUG("btl_download_fw_for_self_ctp:btl_get_fw_checksum error");
			continue;
		}

		if((update_type == FW_ARG_UPDATE)&&(fw_checksum == fw_bin_checksum))
		{
            ret = btl_write_flash_vertify(pfwbin);
			if(ret < 0)
				continue;
		}
		break;
	}

	if(retry < 0)
	{
		BTL_DEBUG("btl_download_fw_for_self_ctp error\n");
		return -1;
	}

	BTL_DEBUG("btl_download_fw_for_self_ctp success___\n");	

	return 0;
}

static int btl_update_flash_for_self_interactive_ctp(unsigned char update_type, unsigned char *pfwbin,int fwsize, int specificArgAddr)
{
	int retry = 0;
	int ret = 0;
	unsigned short fw_checksum = 0x0;
	unsigned short fw_bin_checksum = 0x0;
	retry =3;
	while(retry--)
	{
		SET_WAKEUP_LOW;

	    ret = btl_download_fw_for_self_interactive_ctp(pfwbin,specificArgAddr,fwsize);

		if(ret<0)
		{
			BTL_DEBUG("btl_update_flash_for_self_interactive_ctp:btl_download_fw_for_self_interactive_ctp error retry=%d\n",retry);
			continue;
		}
		
		MDELAY(50);

		SET_WAKEUP_HIGH;
	
	    btl_get_fw_bin_checksum_for_self_interactive_ctp(pfwbin,&fw_bin_checksum, fwsize,specificArgAddr);
	    ret = btl_get_fw_checksum(&fw_checksum);
		fw_checksum -= 0xff;
		BTL_DEBUG("btl_update_flash_for_self_interactive_ctp:fw checksum = 0x%x,fw_bin_checksum =0x%x\n",fw_checksum, fw_bin_checksum);

		
		if((ret < 0) || ((update_type == FW_ARG_UPDATE)&&(fw_checksum != fw_bin_checksum)))
		{
			BTL_DEBUG("btl_update_flash_for_self_interactive_ctp:btl_get_fw_checksum error");
			continue;
		}

		if((update_type == FW_ARG_UPDATE)&&(fw_checksum == fw_bin_checksum))
		{
            ret = btl_write_flash_vertify(pfwbin);
			if(ret < 0)
				continue;
		}
		break;
	}

	if(retry < 0)
	{
		BTL_DEBUG("btl_update_flash_for_self_interactive_ctp error\n");
		return -1;
	}

	BTL_DEBUG("btl_update_flash_for_self_interactive_ctp success___\n");	

	return 0;
}

static int btl_update_flash_for_copatible_ctp(unsigned char update_type, unsigned char *pfwbin,int fwsize)
{
	int retry = 0;
	int ret = 0;
	unsigned short fw_checksum = 0x0;
	unsigned short fw_bin_checksum = 0x0;
	retry =3;
	while(retry--)
	{
		SET_WAKEUP_LOW;

	    ret = btl_download_fw_for_compatible_ctp(pfwbin, fwsize);

		if(ret<0)
		{
			BTL_DEBUG("btl fw update start btl_download_fw error retry=%d\n",retry);
			continue;
		}
		
		MDELAY(50);

		SET_WAKEUP_HIGH;		

	    btl_get_fw_bin_checksum_for_compatible_ctp(pfwbin,&fw_bin_checksum, fwsize);
	    ret = btl_get_fw_checksum(&fw_checksum);
		BTL_DEBUG("btl fw update end,fw checksum = 0x%x,fw_bin_checksum =0x%x\n",fw_checksum, fw_bin_checksum);
		
		if((ret < 0) || ((update_type == FW_ARG_UPDATE)&&(fw_checksum != fw_bin_checksum)))
		{
			BTL_DEBUG("btl fw update start btl_download_fw btl_get_fw_checksum error");
			continue;
		}

		break;
	}

	if(retry < 0)
	{
		BTL_DEBUG("btl fw update error\n");
		return -1;
	}

	BTL_DEBUG("btl fw update success___\n");	

	return 0;
}

static unsigned char choose_update_type_for_self_ctp(unsigned char isBlank, unsigned char* fw_data, unsigned char fwVer, unsigned char arguVer, unsigned short fwChecksum, unsigned short fwBinChecksum,int specifyArgAddr)
{
    unsigned char update_type = NONE_UPDATE;
    if(isBlank)
	{
		update_type = FW_ARG_UPDATE;
		BTL_DEBUG("Update case 0:FW_ARG_UPDATE\n");
	}
	else
	{
        if((fwVer != fw_data[specifyArgAddr + BTL_FWVER_MAIN_OFFSET])
		    ||(arguVer != fw_data[specifyArgAddr + BTL_FWVER_ARGU_OFFSET])
		    ||(fwChecksum != fwBinChecksum))
	    {
            update_type = FW_ARG_UPDATE;
		    BTL_DEBUG("Update case 1:FW_ARG_UPDATE\n");
	    }
	    else
	    {
            update_type = NONE_UPDATE;
            BTL_DEBUG("Update case 4:NONE_UPDATE\n");
	    }
	}
    return update_type;
}

static unsigned char choose_update_type_for_self_interactive_ctp(unsigned char isBlank, unsigned char* fw_data, unsigned char fwVer, unsigned char arguVer, unsigned short fwChecksum, unsigned short fwBinChecksum,int specifyArgAddr)
{
    unsigned char update_type = NONE_UPDATE;
    if(isBlank)
	{
		update_type = FW_ARG_UPDATE;
		BTL_DEBUG("Update case 0:FW_ARG_UPDATE\n");
	}
	else
	{
        if((fwVer != fw_data[specifyArgAddr + BTL_ARGUMENT_BASE_OFFSET + BTL_FWVER_MAIN_OFFSET])
		    ||(arguVer != fw_data[specifyArgAddr + BTL_ARGUMENT_BASE_OFFSET + BTL_FWVER_ARGU_OFFSET])
		    ||(fwChecksum != fwBinChecksum))
	    {
            update_type = FW_ARG_UPDATE;
		    BTL_DEBUG("Update case 1:FW_ARG_UPDATE\n");
	    }
	    else
	    {
            update_type = NONE_UPDATE;
            BTL_DEBUG("Update case 4:NONE_UPDATE\n");
	    }
	}
    return update_type;
}

static unsigned char choose_update_type_for_compatible_ctp(unsigned isBlank, unsigned char* fw_data, unsigned char prjID, unsigned short checksum)
{
    unsigned char update_type = NONE_UPDATE;
    if(isBlank)
	{
		update_type = FW_ARG_UPDATE;
		BTL_DEBUG("Update case 0:FW_ARG_UPDATE\n");
	}
	else
	{
        if((prjID != fw_data[PJ_ID_OFFSET])||(checksum == 0))
	    {
            update_type = FW_ARG_UPDATE;
		    BTL_DEBUG("Update case 1:FW_ARG_UPDATE\n");
	    }
	    else
	    {
            update_type = NONE_UPDATE;
            BTL_DEBUG("Update case 4:NONE_UPDATE\n");
	    }
	}
    return update_type;
}

static int btl_update_fw_for_self_ctp(unsigned char fileType,unsigned char* fw_data, int fw_size)
{
	unsigned char fwArgPrjID[3];    //firmware version/argument version/project identification
	int ret = 0x00;
	unsigned char isBlank = 0x0;    //Indicate the IC have any firmware
	unsigned short fw_checksum = 0x0;  //The checksum for firmware in IC
	unsigned short fw_bin_checksum = 0x0;  //The checksum for firmware in file
	unsigned char update_type = NONE_UPDATE;  
	unsigned int downSize = 0x0;       //The available size of firmware data in file 
	unsigned char cobID[6] = {0};           //The identification for COB project
	unsigned int specificArguAddr = BTL_ARGUMENT_BASE_OFFSET;   //The specific argument base address in firmware date with cobID 
	unsigned char arguCount = 0x0;      //The argument count for COB firmware
	unsigned char IsCobPrj = 0;         //Judge the project type depend firmware file
	unsigned char projectFlag = 0;      //protect flag
    BTL_DEBUG("btl_update_fw_for_self_ctp start\n");	

//check protect flag
    projectFlag = btl_get_protect_flag();
    if(btl_get_protect_flag() && (fileType == HEADER_FILE_UPDATE))
    {
        BTL_DEBUG("btl_update_fw_for_self_ctp:projectFlag = %x, fileType = %x", projectFlag, fileType);
		return 0;
    }
	
//Step 1:Obtain project type
    IsCobPrj = btl_is_cob_project_for_self(fw_data, fw_size);
    BTL_DEBUG("btl_update_fw_for_self_ctp:IsCobPrj = %x", IsCobPrj);

//Step 2:Obtain IC version number
	MDELAY(5);
	ret = btl_get_fwArgPrj_id(fwArgPrjID);
	if((ret < 0)
		|| (fileType == BIN_FILE_UPDATE)
		|| ((ret == 0) && (fwArgPrjID[0] == 0))
		|| ((ret == 0) && (fwArgPrjID[0] == 0xff)) 
		|| ((ret == 0) && (fwArgPrjID[0] == BTL_FWVER_PJ_ID_REG))
		|| (btl_read_flash_vertify(fw_data) < 0))
	{
	    isBlank = 1;
		BTL_DEBUG("btl_update_fw_for_self_ctp:This is blank IC ret = %x fwArgPrjID[0]=%x fwArgPrjID[1]=%x fwArgPrjID[2]=%x\n",ret,fwArgPrjID[0],fwArgPrjID[1], fwArgPrjID[2]);
    }
	else
    {
        isBlank = 0;
		BTL_DEBUG("btl_update_fw_for_self_ctp:ret=%x fwID=%x argID=%x prjID=%x\n",ret,fwArgPrjID[0],fwArgPrjID[1], fwArgPrjID[2]);
    }
    BTL_DEBUG("btl_update_fw_for_self_ctp:isBlank = %x\n",isBlank);
	
//Step 3:Specify download size
    if(IsCobPrj)
    {
    	downSize = btl_get_cob_project_down_size_arguCnt_for_self_ctp(fw_data,fw_size,&arguCount);
    	BTL_DEBUG("btl_update_fw_for_self_ctp:downSize = %x,arguCount = %x\n",downSize,arguCount);
    }
    else
    {
    	downSize = fw_size;
    	BTL_DEBUG("btl_update_fw_for_self_ctp:downSize = %x\n",downSize);
    }

UPDATE_SECOND_FOR_COB:
//Step 4:Update the fwArgPrjID
    if(!isBlank)
    {
        btl_get_fwArgPrj_id(fwArgPrjID);
    }
	
//Step 5:Specify the argument data for cob project
    if(IsCobPrj && !isBlank)
    {
        MDELAY(50);
        ret = btl_get_cob_id(cobID);
		if(ret < 0)
        {
            BTL_DEBUG("btl_update_fw_for_self_ctp:btl_get_cob_id error\n");
			ret = -1;
			goto UPDATE_ERROR;
        }
		else
		{
		    BTL_DEBUG("btl_update_fw_for_self_ctp:cobID = %x %x %x %x %x %x\n",cobID[0],cobID[1],cobID[2],cobID[3],cobID[4],cobID[5]);
		}
		ret = btl_get_specific_argument_for_self_ctp(&specificArguAddr,cobID,fw_data,fw_size,arguCount);
		if(ret < 0)
		{
            BTL_DEBUG("Can't found argument for CTP module,use default argu:\n");
		}
		BTL_DEBUG("btl_update_fw_for_self_ctp:specificArguAddr = %x\n",specificArguAddr);
       }
	
	BTL_DEBUG("fw_data[] = %x  fw_data[] = %x", fw_data[BTL_ARGUMENT_BASE_OFFSET + VERTIFY_START_OFFSET],fw_data[BTL_ARGUMENT_BASE_OFFSET + VERTIFY_END_OFFSET]);
	
//Step 6:Specify whether switch the touch
    BTL_DEBUG("isBlank = %d  ver1 = %d ver2 = %d binVer1 = %d binVer2 = %d specificAddr = %x", isBlank,fwArgPrjID[0],fwArgPrjID[1],fw_data[specificArguAddr+BTL_FWVER_MAIN_OFFSET],fw_data[specificArguAddr + BTL_FWVER_ARGU_OFFSET], specificArguAddr);
    if(!isBlank)
	{
		btl_get_fw_bin_checksum_for_self_ctp(fw_data, &fw_bin_checksum, downSize, specificArguAddr);
		ret = btl_get_fw_checksum(&fw_checksum);
		if((ret < 0) || (fw_checksum != fw_bin_checksum)){
			BTL_DEBUG("btl_update_fw_for_self_ctp:Read checksum fail fw_checksum = %x\n",fw_checksum);
			fw_checksum = 0x00;
		}
		BTL_DEBUG("btl_update_fw_for_self_ctp:fw_checksum = 0x%x,fw_bin_checksum = 0x%x___\n",fw_checksum, fw_bin_checksum);
    }

//Step 7:Select update fw+arg or only update arg
    update_type = choose_update_type_for_self_ctp(isBlank, fw_data, fwArgPrjID[0], fwArgPrjID[1], fw_checksum, fw_bin_checksum,specificArguAddr);

//Step 8:Start Update depend condition
    if(update_type != NONE_UPDATE)
    {
        ret = btl_update_flash_for_self_ctp(update_type, fw_data, downSize,specificArguAddr);
        if(ret < 0)
        {
            BTL_DEBUG("btl_update_fw_for_self_ctp:btl_update_flash failed\n");
			goto UPDATE_ERROR;
        }
    }
	 
//Step 9:Execute second update flow when project firmware is cob and last update_type is FW_ARG_UPDATE
    if((ret == 0)&&(IsCobPrj)&&(isBlank))
    {
        isBlank = 0;
        BTL_DEBUG("btl_update_fw_for_self_ctp:btl_update_flash for COB project need second update with blank IC:isBlank = %d\n",isBlank);
        goto UPDATE_SECOND_FOR_COB;
    }
	BTL_DEBUG("btl_update_fw_for_self_ctp exit\n");

UPDATE_ERROR:
	return ret;
}

static int btl_update_fw_for_self_interactive_ctp(unsigned char fileType,unsigned char* fw_data, int fw_size)
{
	unsigned char fwArgPrjID[3];    //firmware version/argument version/project identification
	int ret = 0x00;
	unsigned char isBlank = 0x0;    //Indicate the IC have any firmware
	unsigned short fw_checksum = 0x0;  //The checksum for firmware in IC
	unsigned short fw_bin_checksum = 0x0;  //The checksum for firmware in file
	unsigned char update_type = NONE_UPDATE;  
	unsigned int downSize = 0x0;       //The available size of firmware data in file 
	unsigned char prjID = {0};           //The identification for COB project
	unsigned int specificArguAddr = 0;   //The specific argument base address in firmware date with cobID 
	unsigned char arguCount = 0x0;      //The argument count for COB firmware
	unsigned char IsCobPrj = 0;         //Judge the project type depend firmware file
	unsigned char projectFlag = 0;      //protect flag
    BTL_DEBUG("btl_update_fw_for_self_interactive_ctp start\n");

//check protect flag
    projectFlag = btl_get_protect_flag();
    if(projectFlag && (fileType == HEADER_FILE_UPDATE))
    {
        BTL_DEBUG("btl_update_fw_for_self_interactive_ctp:projectFlag = %x, fileType = %x", projectFlag, fileType);
		return 0;
    }
	
//Step 1:Obtain project type
    IsCobPrj = btl_is_cob_project_for_self_interactive(fw_data, fw_size);
    BTL_DEBUG("btl_update_fw_for_self_interactive_ctp:IsCobPrj = %x", IsCobPrj);

//Step 2:Obtain IC version number
	MDELAY(5);
	ret = btl_get_fwArgPrj_id(fwArgPrjID);
	if((ret < 0)
		|| (fileType == BIN_FILE_UPDATE)
		|| ((ret == 0) && (fwArgPrjID[0] == 0))
		|| ((ret == 0) && (fwArgPrjID[0] == 0xff)) 
		|| ((ret == 0) && (fwArgPrjID[0] == BTL_FWVER_PJ_ID_REG))
		|| (btl_read_flash_vertify(fw_data) < 0))
	{
	    isBlank = 1;
		BTL_DEBUG("btl_update_fw_for_self_interactive_ctp:This is blank IC ret = %x fwArgPrjID[0]=%x fwArgPrjID[1]=%x fwArgPrjID[2]=%x\n",ret,fwArgPrjID[0],fwArgPrjID[1], fwArgPrjID[2]);
    }
	else
    {
        isBlank = 0;
		BTL_DEBUG("btl_update_fw_for_self_interactive_ctp:ret=%x fwID=%x argID=%x prjID=%x\n",ret,fwArgPrjID[0],fwArgPrjID[1], fwArgPrjID[2]);
    }
    BTL_DEBUG("btl_update_fw_for_self_interactive_ctp:isBlank = %x\n",isBlank);
	
//Step 3:Specify download size
    if(IsCobPrj)
    {
    	downSize = btl_get_cob_project_down_size_arguCnt_for_interactive_ctp(fw_data,fw_size,&arguCount);
    	BTL_DEBUG("btl_update_fw_for_self_interactive_ctp:downSize = %x,arguCount = %x\n",downSize,arguCount);
    }
    else
    {
    	downSize = fw_size;
    	BTL_DEBUG("btl_update_fw_for_self_interactive_ctp:downSize = %x\n",downSize);
    }

UPDATE_SECOND_FOR_COB:
//Step 4:Update the fwArgPrjID
    if(!isBlank)
    {
        btl_get_fwArgPrj_id(fwArgPrjID);
    }
	
//Step 5:Specify the argument data for cob project
    if(IsCobPrj && !isBlank)
    {
        MDELAY(50);
        ret = btl_get_prj_id(&prjID);
		if(ret < 0)
        {
            BTL_DEBUG("btl_update_fw_for_self_interactive_ctp:btl_get_prj_id error\n");
			ret = -1;
			goto UPDATE_ERROR;
        }
		else
		{
		    BTL_DEBUG("btl_update_fw_for_self_interactive_ctp:prjID = %x\n",prjID);
		}
		ret = btl_get_specific_argument_for_self_interactive_ctp(&specificArguAddr,prjID,fw_data,fw_size,arguCount);
		if(ret < 0)
		{
            BTL_DEBUG("Can't found argument for CTP module,use default argu:\n");
		}
		BTL_DEBUG("btl_update_fw_for_self_interactive_ctp:specificArguAddr = %x\n",specificArguAddr);
       }
		
//Step 6:Specify whether switch the touch
    BTL_DEBUG("isBlank = %d  ver1 = %d ver2 = %d binVer1 = %d binVer2 = %d specificAddr = %x", isBlank,fwArgPrjID[0],fwArgPrjID[1],fw_data[specificArguAddr+BTL_ARGUMENT_BASE_OFFSET+BTL_FWVER_MAIN_OFFSET],fw_data[specificArguAddr + BTL_ARGUMENT_BASE_OFFSET + BTL_FWVER_ARGU_OFFSET], specificArguAddr);
    if(!isBlank)
	{
		btl_get_fw_bin_checksum_for_self_interactive_ctp(fw_data, &fw_bin_checksum, downSize, specificArguAddr);
		ret = btl_get_fw_checksum(&fw_checksum);
		if((ret < 0) || (fw_checksum != fw_bin_checksum)){
			BTL_DEBUG("btl_update_fw_for_self_interactive_ctp:Read checksum fail fw_checksum = %x\n",fw_checksum);
			fw_checksum = 0x00;
		}
		BTL_DEBUG("btl_update_fw_for_self_interactive_ctp:fw_checksum = 0x%x,fw_bin_checksum = 0x%x___\n",fw_checksum, fw_bin_checksum);
    }

//Step 7:Select update fw+arg or only update arg
    update_type = choose_update_type_for_self_interactive_ctp(isBlank, fw_data, fwArgPrjID[0], fwArgPrjID[1], fw_checksum, fw_bin_checksum,specificArguAddr);

//Step 8:Start Update depend condition
    if(update_type != NONE_UPDATE)
    {
        ret = btl_update_flash_for_self_interactive_ctp(update_type, fw_data, downSize,specificArguAddr);
        if(ret < 0)
        {
            BTL_DEBUG("btl_update_fw_for_self_interactive_ctp:btl_update_flash failed\n");
			goto UPDATE_ERROR;
        }
    }
	 
//Step 9:Execute second update flow when project firmware is cob and last update_type is FW_ARG_UPDATE
    if((ret == 0)&&(IsCobPrj)&&(isBlank))
    {
        isBlank = 0;
        BTL_DEBUG("btl_update_fw_for_self_interactive_ctp:btl_update_flash for COB project need second update with blank IC:isBlank = %d\n",isBlank);
        goto UPDATE_SECOND_FOR_COB;
    }
	BTL_DEBUG("btl_update_fw_for_self_interactive_ctp exit\n");

UPDATE_ERROR:
	return ret;
}

static int btl_update_fw_for_compatible_ctp(unsigned char fileType,unsigned char* fw_data, int fw_size)
{
	unsigned char fwArgPrjID;	//firmware version
	int ret = 0x00;
	unsigned short fw_bin_checksum = 0x0;
	unsigned short fw_checksum = 0x0;
	unsigned char isBlank = 0x0;	//Indicate the IC have any firmware
	unsigned char update_type = NONE_UPDATE;  
	BTL_DEBUG("btl_update_fw_for_compatible_ctp start\n");
	
//Step 1:Obtain IC version number
	MDELAY(5);
	ret = btl_get_prj_id(&fwArgPrjID);
	if((ret < 0)
		|| (fileType == BIN_FILE_UPDATE)
		|| ((ret == 0) && (fwArgPrjID == 0))
		|| ((ret == 0) && (fwArgPrjID == 0xff)) 
		|| ((ret == 0) && (fwArgPrjID == BTL_PRJ_ID_REG)))
	{
		isBlank = 1;
		BTL_DEBUG("btl_update_fw_for_compatible_ctp:This is blank IC ret = %x fwArgPrjID=%x\n",ret,fwArgPrjID);
	}
	else
	{
		isBlank = 0;
		BTL_DEBUG("btl_update_fw_for_compatible_ctp:ret=%x fwID=%x\n",ret,fwArgPrjID);
	}
	BTL_DEBUG("btl_update_fw_for_compatible_ctp:isBlank = %d  fwID = %d binFwID = %d", isBlank,fwArgPrjID,fw_data[BTL_PRJ_ID_REG]);
//step 2:check checksum

    BTL_DEBUG("isBlank = %d  fwArgPrjID = %d  binFwArgPrjID = %d\n", isBlank,fwArgPrjID, fw_data[PJ_ID_OFFSET]);
    if(!isBlank)
    {
    	btl_get_fw_bin_checksum_for_compatible_ctp(fw_data, &fw_bin_checksum, fw_size);
    	ret = btl_get_fw_checksum(&fw_checksum);
    	if((ret < 0) || (fw_checksum != fw_bin_checksum)){
    		BTL_DEBUG("btl_update_fw_for_compatible_ctp:Read checksum fail fw_checksum = %x\n",fw_checksum);
    		fw_checksum = 0x00;
    	}
    	BTL_DEBUG("btl_update_fw_for_compatible_ctp:fw_checksum = 0x%x,fw_bin_checksum = 0x%x___\n",fw_checksum, fw_bin_checksum);
    }

//Step 2:Select update fw+arg or only update arg
	update_type = choose_update_type_for_compatible_ctp(isBlank, fw_data, fwArgPrjID, fw_checksum);

//Step 3:Start Update depend condition
	if(update_type != NONE_UPDATE)
	{
		ret = btl_update_flash_for_copatible_ctp(update_type, fw_data, fw_size);
		if(ret < 0)
		{
			BTL_DEBUG("btl_update_fw_for_compatible_ctp:btl_update_flash failed\n");
			goto UPDATE_ERROR;
		}
	}
	 
UPDATE_ERROR:
	BTL_DEBUG("btl_update_fw_for_compatible_ctp exit\n");
	return ret;
}

static int btl_update_fw(unsigned char fileType,unsigned char ctpType, unsigned char* pFwData, unsigned int fwLen)
{
    int ret = 0;
	BTL_DEBUG("btl_update_fw:ctpType = %x\n",ctpType); 
    switch(ctpType)
    {
        case SELF_CTP:
			ret = btl_update_fw_for_self_ctp(fileType,pFwData,fwLen);
			break;
        case SELF_INTERACTIVE_CTP:
			ret = btl_update_fw_for_self_interactive_ctp(fileType,pFwData,fwLen);
			break;

		case COMPATIBLE_CTP:
			ret = btl_update_fw_for_compatible_ctp(fileType,pFwData,fwLen);
			break;			
    }
	return ret;
}

#ifdef BTL_AUTO_UPDATE_FARMWARE
int btl_auto_update_fw(void)
{
	int ret = 0;
    unsigned int fwLen = sizeof(fwbin);

	BTL_DEBUG("btl_auto_update_fw:fwLen = %x\n",fwLen); 
	g_btl_ts->enter_update = 1;
	ret = btl_update_fw(HEADER_FILE_UPDATE,CTP_TYPE, (unsigned char *)fwbin, fwLen);
	
	if(ret < 0)
	{
		BTL_DEBUG("btl_auto_update_fw: btl_update_fw fail\n");  
	}
	else
	{
		BTL_DEBUG("btl_auto_update_fw: btl_update_fw success\n");  
	}
	g_btl_ts->enter_update = 0;
	return ret;
}
#endif

#ifdef BTL_UPDATE_FARMWARE_WITH_BIN
static int btl_GetFirmwareSize(char *firmware_path)
{
       struct file *pfile = NULL;
       struct inode *inode;
       unsigned long magic;
       off_t fsize = 0;

       if (NULL == pfile)
               pfile = filp_open(firmware_path, O_RDONLY, 0);
       if (IS_ERR(pfile)) 
       {
               pr_err("error occured while opening file %s.\n", firmware_path);
               return -EIO;
       }

       inode = file_inode(pfile);
       magic = inode->i_sb->s_magic;
       fsize = inode->i_size;
       filp_close(pfile, NULL);

       return fsize;
}

static int btl_ReadFirmware(char *firmware_path, unsigned char *firmware_buf)
{
	struct file *pfile = NULL;
	struct inode *inode;
	unsigned long magic;
	off_t fsize;
	loff_t pos;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
	mm_segment_t old_fs;
#endif

	if (NULL == pfile)
		pfile = filp_open(firmware_path, O_RDONLY, 0);

	if (IS_ERR(pfile)) 
	{
		BTL_DEBUG("error occured while opening file %s. %ld\n", firmware_path, PTR_ERR(pfile));
		return -EIO;
	}

	inode = file_inode(pfile);
	magic = inode->i_sb->s_magic;
	fsize = inode->i_size;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
	old_fs = get_fs();
	set_fs(KERNEL_DS);
#endif

	pos = 0;
	vfs_read(pfile, firmware_buf, fsize, &pos);
	filp_close(pfile, NULL);

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
	set_fs(old_fs);
#endif   
	return 0;
}

int btl_fw_upgrade_with_bin_file(unsigned char *firmware_path)
{
	unsigned char *pbt_buf = NULL;
	int ret = 0;
	int fwsize = 0;

	//Obtain the size of firmware bin
	fwsize = btl_GetFirmwareSize(firmware_path);
	if(fwsize <= 0){
		BTL_DEBUG("Get firmware size error:%d\n",fwsize);
		return -1;
	}
    BTL_DEBUG("%s:Get firmware size %x\n", __func__, fwsize);

	pbt_buf = kmalloc(fwsize + 1, GFP_KERNEL);
	if(pbt_buf == NULL){
		BTL_DEBUG("%s ERROR:Get buf failed\n",__func__);
		return -1;
	}

	ret = btl_ReadFirmware(firmware_path, pbt_buf);
	if (ret < 0) 
	{
		BTL_DEBUG("%s ERROR:Read firmware data failed\n",__func__);
		kfree(pbt_buf);
		return -EIO;
	}

	//Call the upgrade procedure
    ret = btl_update_fw(BIN_FILE_UPDATE, CTP_TYPE, pbt_buf, fwsize);
	kfree(pbt_buf);
	return ret;
}
#endif

#if defined(BTL_UPDATE_FIRMWARE_WITH_REQUEST_FIRMWARE)
int btl_update_firmware_via_request_firmware(void)
{
    int ret = 0;
    const struct firmware *fw = NULL;
	unsigned char *fwData = NULL;
    struct i2c_client* client = g_btl_ts->client;
    struct device *dev = &client->dev;

	BTL_DEBUG_FUNC();
	ret = request_firmware(&fw, BTL_REQUEST_FIRMWARE_BIN_PATH, dev);
	if(ret == 0)
    {
        BTL_DEBUG("request firmware %s success", BTL_REQUEST_FIRMWARE_BIN_PATH);
        fwData = vmalloc(fw->size);
        if (NULL == fwData) {
            BTL_DEBUG("fwData buffer vmalloc fail");
            ret = -ENOMEM;
        } else {
            memcpy(fwData, fw->data, fw->size);
			ret = btl_update_fw(BIN_FILE_UPDATE, CTP_TYPE, fwData, fw->size);
			if(ret < 0) {
				BTL_DEBUG("update firmware fail");
            } else {
                BTL_DEBUG("update firmware success");
            }
        }
    }

	if (fwData != NULL)
    {
        vfree(fwData);
		fwData = NULL;
    }
	
    if (fw != NULL) {
        release_firmware(fw);
        fw = NULL;
    }
	
	return ret;
}
#endif
#endif
