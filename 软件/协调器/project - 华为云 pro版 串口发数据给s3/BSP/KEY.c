#include "KEY.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "OLED.h"

extern  TIM_HandleTypeDef htim1;

uint8_t thsl_i;
extern uint8_t temp_max_1, humi_min_1, light_min;
extern uint8_t temp_max_2, humi_min_2, t_humi_min;
extern float t_ph_max;
extern uint32_t CO2_max;
extern uint8_t fan_state_1, water_state_1, canopy_state, canopy_flag;
extern uint8_t fan_state_2, water_state_2, pump_state;
extern uint8_t mode;

struct keys key[KEY_MAX_NUMBER];

/**
 * @brief  按键初始化：启动定时器中断（用于周期性扫描按键，防抖）
 * @param  无
 * @retval 无
 */
void KEY_Init(void)
{
    HAL_TIM_Base_Start_IT(&htim1);  // 启动定时器1中断（需提前配置10ms周期）
}

/**
 * @brief  按键扫描逻辑：处理5个按键的触发，控制模式切换、参数调整、设备开关
 * @param  无
 * @retval 无
 */
void KEY_Scan(void)
{
    /* 按键0（key[0]）：切换光标索引thsl_i（不同模式下范围不同） */
    if (key[0].key_flag == 1)
    {
        OLED_Clear();  // 清屏，刷新光标位置
        
        // mode0（初始界面）：选择进入监测或设置（光标0/1切换）
        if (mode == 0)
        {
            thsl_i = (thsl_i == 1) ? 0 : 1;  // 0→1→0循环
        }
        // mode2（参数设置）：4个参数切换
        else if (mode == 2)
        {
            thsl_i = (thsl_i == 5) ? 0 : thsl_i + 1;  // 0→1→2→3→0循环
        }
        // mode6（设备控制）：2个设备切换
        else if (mode == 6)
        {
            thsl_i = (thsl_i == 1) ? 0 : thsl_i + 1;  // 0→1→0循环（只有2个设备）
        }
        
        key[0].key_flag = 0;
    }

    /* 按键1（key[1]）：从初始界面（mode0）进入监测界面 */
    else if (key[1].key_flag == 1)
    {
        OLED_Clear();
        
        if (mode == 0)
        {
            mode = 1;  // 直接进入监测界面
        }
        
        key[1].key_flag = 0;
    }

    /* 按键2（key[2]）：模式流转 */
    else if (key[2].key_flag == 1)
    {
        OLED_Clear();
        thsl_i = 0;
        
        // 简化流程：mode0→mode1→mode2→mode6→mode0
        if (mode == 0)
        {
            mode = 1;  // 进入监测界面
        }
        else if (mode == 1)
        {
            mode = 2;  // 监测→参数设置
        }
        else if (mode == 2)
        {
            mode = 6;  // 参数设置→设备控制
        }
        else if (mode == 6)
        {
            mode = 0;  // 返回初始界面
        }
        
        key[2].key_flag = 0;
    }

    /* 按键3（key[3]）：增加参数阈值或开启设备 */
    else if (key[3].key_flag == 1)
    {
        // mode2：参数设置
        if (mode == 2)
        {
            switch (thsl_i)
            {
                case 0: temp_max_2++; break;          // 温度上限+1
                case 1: humi_min_2++; break;          // 湿度下限+1
                case 2: light_min++; break;           // 光照下限+1
                case 3: CO2_max += 100; break;        // CO2上限+100
                case 4: t_ph_max += 0.1f; break;      // PH上限+0.1（新增）
			  		 		case 5: t_humi_min++; break;          // 土湿下限+1（新增）
						}
            OLED_Clear();
            Page();
        }
        // mode6：设备控制
        else if (mode == 6)
        {
            switch (thsl_i)
            {
                case 0: water_state_2 = 1; break;     // 水阀开启
                case 1: pump_state = 1; break;        // 水泵开启
            }
            OLED_Clear();
            Page();
        }
        
        key[3].key_flag = 0;
    }

    /* 按键4（key[4]）：减少参数阈值或关闭设备 */
    else if (key[4].key_flag == 1)
    {
        // mode2：参数设置
        if (mode == 2)
        {
            switch (thsl_i)
            {
                case 0: if (temp_max_2 > 10) temp_max_2--; break;
                case 1: if (humi_min_2 > 20) humi_min_2--; break;
                case 2: if (light_min > 0) light_min--; break;
                case 3: if (CO2_max > 400) CO2_max -= 100; break;
						    case 4: if (t_ph_max > 0) t_ph_max -= 0.1f; break;  // PH下限0（新增）
                case 5: if (t_humi_min > 0) t_humi_min--; break;    // 土湿下限0（新增）
						}
            OLED_Clear();
            Page();
        }
        // mode6：设备控制
        else if (mode == 6)
        {
            switch (thsl_i)
            {
                case 0: water_state_2 = 0; break;     // 水阀关闭
                case 1: pump_state = 0; break;        // 水泵关闭
            }
            OLED_Clear();
            Page();
        }
        
        key[4].key_flag = 0;
    }

    /* 无按键触发：清除所有标志 */
    else
    {
        for(int i = 0; i < KEY_MAX_NUMBER; i++)
        {
            key[i].key_flag = 0;
        }
    }
}