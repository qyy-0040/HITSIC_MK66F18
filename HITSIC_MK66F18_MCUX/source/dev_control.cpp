#include "dev_control.hpp"

/**控制环PITMGR任务句柄**/
pitMgr_t* ctrl_spdCtrlHandle = nullptr;
pitMgr_t* ctrl_dirCtrlHandle = nullptr;
pitMgr_t* ctrl_emCtrlHandle = nullptr;
pitMgr_t* ctrl_strCtrlHandle = nullptr;
pitMgr_t* ctrl_spdtestCtrlHandle = nullptr;

/**控制环初始化**/
void CTRL_Init(void)
{
    ctrl_dirCtrlHandle = pitMgr_t::insert(CTRL_DIR_CTRL_MS, 3U, CTRL_DirCtrl, pitMgr_t::enable);
    assert(ctrl_dirCtrlHandle);
    ctrl_spdCtrlHandle = pitMgr_t::insert(CTRL_SPD_CTRL_MS, 4U, CTRL_SpdCtrl, pitMgr_t::enable);
    assert(ctrl_spdCtrlHandle);
    ctrl_emCtrlHandle = pitMgr_t::insert(CTRL_EM_CTRL_MS, 5U, EM_ErrorUpdate, pitMgr_t::enable);
    assert(ctrl_emCtrlHandle);
    ctrl_strCtrlHandle = pitMgr_t::insert(CTRL_STR_CTRL_MS, 6U, CTRL_StrCtrl, pitMgr_t::enable);
    assert(ctrl_strCtrlHandle);
    ctrl_spdtestCtrlHandle = pitMgr_t::insert(2500, 7U, CTRL_SpdtestCtrl, pitMgr_t::enable);
    assert(ctrl_spdtestCtrlHandle);
}

/**外部变量引用**/
extern uint32_t threshold;      ///< 图像阈值
extern uint32_t preview ;       ///< 图像前瞻
extern float img_error;         ///< 图像error
extern float ema_error;         ///< 电磁error
extern float AD[ChannelTimes];  ///< 图像阈值
extern uint32_t ema_tol;        ///< 电磁测试用参数
extern uint32_t ema_mode;
extern uint32_t ema_detla;
extern bool zebra_stop;

/**全局变量声明**/
float ctrl_sevromid;                ///< 舵机中值
bool ctrl_startstaus = false;       ///< 启动状态
int32_t ctrl_strEn[3] = {0, 0, 1}; ///< 启动使能
int32_t ctrl_testEn[3] = {0, 0, 1}; ///< 测试模式使能
int32_t ctrl_strcountdown = 0U;

/**菜单项初始化**/
void CTRL_MenuInit(menu_list_t *menuList)
{
    static menu_list_t *settingMenuList =
            MENU_ListConstruct("Setting", 20, menuList);   ///< 通用设置
    assert(settingMenuList);
    static menu_list_t *ctrlMenuList =
            MENU_ListConstruct("Control", 32, menuList);   ///< 控制参数调节
    assert(ctrlMenuList);
    static menu_list_t *imageMenuList =
            MENU_ListConstruct("Img&Ema", 20, menuList);   ///< 图像及电磁设置
    assert(imageMenuList);
    static menu_list_t *paraMenuList =
            MENU_ListConstruct("Parameter", 20, menuList); ///< 参数显示
    assert(paraMenuList);
    MENU_ListInsert(menuList, MENU_ItemConstruct(menuType, settingMenuList, "Setting", 0, 0));
    {
        /*MENU_ListInsert(settingMenuList, MENU_ItemConstruct(procType, CTRL_Start, "start/stop", 0U,
                menuItem_proc_runOnce));*/
        MENU_ListInsert(settingMenuList, MENU_ItemConstruct(variType, &ctrl_strEn[0], "start/stop", 0U,
                menuItem_data_NoSave | menuItem_data_NoLoad | menuItem_dataExt_HasMinMax));
        MENU_ListInsert(settingMenuList, MENU_ItemConstruct(variType, &ctrl_testEn[0], "mode.test", 0U,
                menuItem_data_NoSave | menuItem_data_NoLoad | menuItem_dataExt_HasMinMax));
        MENU_ListInsert(settingMenuList, MENU_ItemConstruct(variType, &ctrl_modesel[0], "mode.sel", 0U,
                menuItem_data_NoSave | menuItem_data_NoLoad | menuItem_dataExt_HasMinMax));
        /*MENU_ListInsert(ctrlMenuList, MENU_ItemConstruct(variType, &ctrl_autoselEn[0], "mode.auto", 0U,
                menuItem_data_NoSave | menuItem_data_NoLoad | menuItem_dataExt_HasMinMax));
        MENU_ListInsert(ctrlMenuList, MENU_ItemConstruct(varfType, &ctrl_errtor, "errtor", 11U,
                menuItem_data_region));
        MENU_ListInsert(settingMenuList, MENU_ItemConstruct(variType, &ctrl_spdCtrlEn[0], "spd.en", 0U,
                menuItem_data_NoSave | menuItem_data_NoLoad | menuItem_dataExt_HasMinMax));
        MENU_ListInsert(settingMenuList, MENU_ItemConstruct(variType, &ctrl_dirCtrlEn[0], "dir.en", 0U,
                menuItem_data_NoSave | menuItem_data_NoLoad | menuItem_dataExt_HasMinMax));*/

    }
    MENU_ListInsert(menuList, MENU_ItemConstruct(menuType, ctrlMenuList, "Control", 0, 0));
    {
        /*MENU_ListInsert(ctrlMenuList, MENU_ItemConstruct(varfType, &ctrl_spdSet, "spdSet", 11U,
                menuItem_data_region)); 测试*/
        MENU_ListInsert(ctrlMenuList, MENU_ItemConstruct(varfType, &ctrl_sevromid, "sevro.mid", 20U,
                menuItem_data_region));

        MENU_ListInsert(ctrlMenuList, MENU_ItemConstruct(nullType, NULL, "DIRECTION", 0, 0));
        MENU_ListInsert(ctrlMenuList, MENU_ItemConstruct(varfType, &ctrl_dirPid[CTRL_MODE_IMG].kp, "img.kp", 21U,
                menuItem_data_region));
        MENU_ListInsert(ctrlMenuList, MENU_ItemConstruct(varfType, &ctrl_dirPid[CTRL_MODE_IMG].ki, "img.ki", 22U,
                menuItem_data_region));
        MENU_ListInsert(ctrlMenuList, MENU_ItemConstruct(varfType, &ctrl_dirPid[CTRL_MODE_IMG].kd, "img.kd", 23U,
                menuItem_data_region));
        MENU_ListInsert(ctrlMenuList, MENU_ItemConstruct(varfType, &ctrl_dirPid[CTRL_MODE_EMA].kp, "ema.kp", 24U,
                menuItem_data_region));
        MENU_ListInsert(ctrlMenuList, MENU_ItemConstruct(varfType, &ctrl_dirPid[CTRL_MODE_EMA].ki, "ema.ki", 25U,
                menuItem_data_region));
        MENU_ListInsert(ctrlMenuList, MENU_ItemConstruct(varfType, &ctrl_dirPid[CTRL_MODE_EMA].kd, "ema.kd", 26U,
                menuItem_data_region));

        MENU_ListInsert(ctrlMenuList, MENU_ItemConstruct(nullType, NULL, "SPEED", 0, 0));
        MENU_ListInsert(ctrlMenuList, MENU_ItemConstruct(varfType, &ctrl_spdPid[0].kp, "spdL.kp", 12U,
                menuItem_data_region));
        MENU_ListInsert(ctrlMenuList, MENU_ItemConstruct(varfType, &ctrl_spdPid[0].ki, "spdL.ki", 13U,
                menuItem_data_region));
        MENU_ListInsert(ctrlMenuList, MENU_ItemConstruct(varfType, &ctrl_spdPid[0].kd, "spdL.kd", 14U,
                menuItem_data_region));
        MENU_ListInsert(ctrlMenuList, MENU_ItemConstruct(varfType, &ctrl_spdPid[1].kp, "spdR.kp", 15U,
                menuItem_data_region));
        MENU_ListInsert(ctrlMenuList, MENU_ItemConstruct(varfType, &ctrl_spdPid[1].ki, "spdR.ki", 16U,
                menuItem_data_region));
        MENU_ListInsert(ctrlMenuList, MENU_ItemConstruct(varfType, &ctrl_spdPid[1].kd, "spdR.kd", 17U,
                menuItem_data_region));
    }

    MENU_ListInsert(menuList, MENU_ItemConstruct(menuType, imageMenuList, "Img&Ema", 0, 0));
    {
        MENU_ListInsert(imageMenuList, MENU_ItemConstruct(nullType, NULL, "IMG", 0, 0));
        MENU_ListInsert(imageMenuList, MENU_ItemConstruct(variType, &threshold, "threshold", 30U,
                menuItem_data_global));
        MENU_ListInsert(imageMenuList, MENU_ItemConstruct(variType, &preview, "preview", 31U,
                menuItem_data_global));
        MENU_ListInsert(imageMenuList, MENU_ItemConstruct(varfType, &img_error, "img.error", 0U,
                menuItem_data_NoSave | menuItem_data_NoLoad|menuItem_data_ROFlag));

        MENU_ListInsert(imageMenuList, MENU_ItemConstruct(nullType, NULL, "EMA", 0, 0));
        MENU_ListInsert(imageMenuList, MENU_ItemConstruct(variType, &ema_error, "ema.error", 0U,
                menuItem_data_NoSave | menuItem_data_NoLoad|menuItem_data_ROFlag));

        MENU_ListInsert(imageMenuList, MENU_ItemConstruct(nullType, NULL, "EMAtest", 0, 0));
        MENU_ListInsert(imageMenuList, MENU_ItemConstruct(variType, &ema_tol, "ema_tol", 32U,
                menuItem_data_global));
        MENU_ListInsert(imageMenuList, MENU_ItemConstruct(variType, &ema_detla, "ema.detla", 0U,
                menuItem_data_NoSave | menuItem_data_NoLoad|menuItem_data_ROFlag));
        MENU_ListInsert(imageMenuList, MENU_ItemConstruct(variType, &ema_mode, "ema.mode", 0U,
                menuItem_data_NoSave | menuItem_data_NoLoad|menuItem_data_ROFlag));
    }

    MENU_ListInsert(menuList, MENU_ItemConstruct(menuType, paraMenuList, "Parameter", 0, 0));
    {
        MENU_ListInsert(paraMenuList, MENU_ItemConstruct(nullType, NULL, "STAUS", 0, 0));
        MENU_ListInsert(paraMenuList, MENU_ItemConstruct(variType, &ctrl_mode, "mode", 0U,
                menuItem_data_NoSave | menuItem_data_NoLoad | menuItem_data_ROFlag));

        MENU_ListInsert(paraMenuList, MENU_ItemConstruct(nullType, NULL, "DIRECTION", 0, 0));
        MENU_ListInsert(paraMenuList, MENU_ItemConstruct(varfType, &img_error, "img.error", 0U,
                menuItem_data_NoSave | menuItem_data_NoLoad|menuItem_data_ROFlag));
        MENU_ListInsert(paraMenuList, MENU_ItemConstruct(varfType, &ema_error, "ema.error", 0U,
                menuItem_data_NoSave | menuItem_data_NoLoad|menuItem_data_ROFlag));
        MENU_ListInsert(paraMenuList, MENU_ItemConstruct(varfType, &ctrl_sevromid, "servo.mid", 0U,
                menuItem_data_NoSave | menuItem_data_NoLoad|menuItem_data_ROFlag));
        MENU_ListInsert(paraMenuList, MENU_ItemConstruct(varfType, &ctrl_dirPidOutput, "servo.out", 0U,
                menuItem_data_NoSave | menuItem_data_NoLoad|menuItem_data_ROFlag));

        MENU_ListInsert(paraMenuList, MENU_ItemConstruct(nullType, NULL, "SPEED", 0, 0));
        MENU_ListInsert(paraMenuList, MENU_ItemConstruct(varfType, &ctrl_spdSet, "spd.set", 0U,
                menuItem_data_NoSave | menuItem_data_NoLoad));
        MENU_ListInsert(paraMenuList, MENU_ItemConstruct(varfType, &ctrl_spdL, "spdL.cur", 0U,
                menuItem_data_NoSave | menuItem_data_NoLoad));
        MENU_ListInsert(paraMenuList, MENU_ItemConstruct(varfType, &ctrl_spdR, "spdR.cur", 0U,
                menuItem_data_NoSave | menuItem_data_NoLoad));

        MENU_ListInsert(paraMenuList, MENU_ItemConstruct(nullType, NULL, "AD", 0, 0));
        MENU_ListInsert(paraMenuList, MENU_ItemConstruct(varfType, &AD[0], "AD[0]", 0U,
                menuItem_data_NoSave | menuItem_data_NoLoad|menuItem_data_ROFlag));
        MENU_ListInsert(paraMenuList, MENU_ItemConstruct(varfType, &AD[1], "AD[1]", 0U,
                menuItem_data_NoSave | menuItem_data_NoLoad|menuItem_data_ROFlag));
        MENU_ListInsert(paraMenuList, MENU_ItemConstruct(varfType, &AD[2], "AD[2]", 0U,
                menuItem_data_NoSave | menuItem_data_NoLoad|menuItem_data_ROFlag));
        MENU_ListInsert(paraMenuList, MENU_ItemConstruct(varfType, &AD[3], "AD[3]", 0U,
                menuItem_data_NoSave | menuItem_data_NoLoad|menuItem_data_ROFlag));
        MENU_ListInsert(paraMenuList, MENU_ItemConstruct(varfType, &AD[4], "AD[4]", 0U,
                menuItem_data_NoSave | menuItem_data_NoLoad|menuItem_data_ROFlag));
        MENU_ListInsert(paraMenuList, MENU_ItemConstruct(varfType, &AD[5], "AD[5]", 0U,
                menuItem_data_NoSave | menuItem_data_NoLoad|menuItem_data_ROFlag));
        MENU_ListInsert(paraMenuList, MENU_ItemConstruct(varfType, &AD[6], "AD[6]", 0U,
                menuItem_data_NoSave | menuItem_data_NoLoad|menuItem_data_ROFlag));
        MENU_ListInsert(paraMenuList, MENU_ItemConstruct(varfType, &AD[7], "AD[7]", 0U,
                menuItem_data_NoSave | menuItem_data_NoLoad|menuItem_data_ROFlag));
    }
}

/* ******************** 启动延时 ******************** */
void CTRL_StrCtrl(void *userData)
{
    if(1 == ctrl_strEn[0] && ctrl_startstaus == false)
    {
        ctrl_strcountdown += 20;
        if(ctrl_strcountdown == 2000)
        {
            ctrl_startstaus = true;
        }

    }
    if(0 == ctrl_strEn[0])
    {
        ctrl_strcountdown = 0;
        ctrl_startstaus = false;
    }
}
/*void CTRL_Start(void)
{
    if(ctrl_startstaus == false)
    {
        DISP_SSD1306_delay_ms(1500);///< 延时
        ctrl_startstaus = !ctrl_startstaus;
    }
    else
    {
        ctrl_startstaus = !ctrl_startstaus;///< 启动/停止
    }
}*/

/* *********************************************** */

/* ******************** 速度环 ******************** */
float ctrl_motorL[2];
float ctrl_motorR[2];

float ctrl_spdSet = 0.0f; ///< 速度设置

pidCtrl_t ctrl_spdPid[2];

float ctrl_spdL = 0.0f, ctrl_spdR = 0.0f;
float ctrl_spdLerror = 0.0f, ctrl_spdRerror = 0.0f;
float ctrl_spdLOutput = 0.0f; ///< 速度环输出
float ctrl_spdROutput = 0.0f;
uint32_t ctrl_testcount = 0;
float ctrl_spdtest[2] = {1.0, 2.5};
void CTRL_SpdtestCtrl(void *userData)
{
    if((ctrl_startstaus) || (1 == ctrl_testEn[0]/* && !zebra_stop*/))
    {
        ctrl_testcount++;
        ctrl_spdSet = ctrl_spdtest[ctrl_testcount%2];
    }
    else
    {
        ctrl_testcount = 0;//测试用
    }
}

bool CTRL_Protect(int32_t ctrl_mode)  ///< 出赛道保护（图像部分未完成）
{
    /**图像模式**/
    if(ctrl_mode == CTRL_MODE_IMG)
    {
        return false;
    }
    /**电磁模式**/
    else if(ctrl_mode == CTRL_MODE_EMA)
    {
        if(AD[1] == 1 && AD[7] == 1)
        {
            return true; ///< 出赛道返回真
        }
    }
    return false; ///< 未出赛道返回假
}
void CTRL_SpdCtrl(void *userData)   ///< 速度环主函数
{
    /**已启动且速度环已使能更新电机输出**/
    if((ctrl_startstaus) || (1 == ctrl_testEn[0]/* && !zebra_stop*/))
    {
        /**出赛道电机停转**/
        if(CTRL_Protect(ctrl_mode))
        {
            ctrl_spdLOutput = 0;
            ctrl_spdROutput = 0;
        }
        else
        {
            ctrl_spdL = ((float)SCFTM_GetSpeed(ENCO_R_PERIPHERAL)) * CTRL_ENCO_SPD_COEFF;
            SCFTM_ClearSpeed(ENCO_R_PERIPHERAL);
            ctrl_spdR = -((float)SCFTM_GetSpeed(ENCO_L_PERIPHERAL)) * CTRL_ENCO_SPD_COEFF;
            SCFTM_ClearSpeed(ENCO_L_PERIPHERAL);
            PIDCTRL_ErrUpdate(&ctrl_spdPid[0], ctrl_spdSet - ctrl_spdL);
            ctrl_spdLOutput += PIDCTRL_DeltaPIGain(&ctrl_spdPid[0]);
            PIDCTRL_ErrUpdate(&ctrl_spdPid[1], ctrl_spdSet - ctrl_spdR);
            ctrl_spdROutput += PIDCTRL_DeltaPIGain(&ctrl_spdPid[1]);
        }
    }
    /**未启动时电机停转**/
    else
    {
        ctrl_spdLOutput = 0;
        ctrl_spdROutput = 0;
    }
    CTRL_MotorUpdate(ctrl_spdLOutput, ctrl_spdROutput);
}

/* *********************************************** */

/* ******************** 转向环 ******************** */
int32_t ctrl_modesel[3] = {0, 0, 1};   ///< 模式选择
//int32_t ctrl_autoselEn[3] = {0, 0, 1}; ///< 自动模式使能
pidCtrl_t ctrl_dirPid[2];

float ctrl_dirPidOutput = 0.0f; ///< 转向环输出
float ctrl_errtor = 0.0f;
int32_t ctrl_mode = 0U;  ///< 目前模式

int32_t CTRL_GetCtrlMode(void)  ///< 获取当前模式
{
    if(1 == ctrl_autoselEn[0])///< 检测自动模式使能
    {
        /**直道用电磁**/
        if(img_error < ctrl_errtor && ctrl_modesel[0] == 0)
        {
            ctrl_modesel[0]++;
        }
        /**弯道用图像**/
        else if(img_error > ctrl_errtor && ctrl_modesel[0] == 1)
        {
            ctrl_modesel[0]--;
        }
    }
}

float CTRL_GetDirError(int ctrl_dirmode)  ///< 根据当前模式获取error
{
    if(ctrl_mode == CTRL_MODE_IMG)
    {
        return img_error;
    }
    else if(ctrl_mode == CTRL_MODE_EMA)
    {
        return ema_error;
    }
    return 0;
}

//void CTRL_CheckZebra(void)
//{
    //if(zebra_stop)
    //{
        //ctrl_startstaus = false;
    //}
//}

void CTRL_DirCtrl(void *userData)   ///< 方向环主函数
{
    /**已启动且方向环已使能更新舵机输出**/
    if((ctrl_startstaus) || (1 == ctrl_testEn[0]/* && !zebra_stop*/))
    {
        //CTRL_GetCtrlMode();            ///< 获取当前模式
        ctrl_mode = ctrl_modesel[0];   ///< 更新当前模式
        //CTRL_CheckZebra();
        PIDCTRL_ErrUpdate(&ctrl_dirPid[ctrl_mode], (CTRL_GetDirError(ctrl_mode)));  ///< 更新PID
        ctrl_dirPidOutput = ctrl_sevromid + PIDCTRL_CalcPIDGain(&ctrl_dirPid[ctrl_mode]);  ///< 舵机限位
    }
    /**未启动舵机输出中值**/
    else
    {
        ctrl_dirPidOutput = ctrl_sevromid;
    }
    ctrl_dirPidOutput = CTRL_SevroUpdate(ctrl_dirPidOutput);  ///< 记录当前舵机输出值
    SCFTM_PWM_ChangeHiRes(FTM3,kFTM_Chnl_7,50U,ctrl_dirPidOutput); ///< 舵机输出
}

/* *********************************************** */

float CTRL_SevroUpdate(float sevro)  ///< 舵机限位
{
    /**大于最大值则输出最大值**/
    if(sevro > ctrl_sevromid+0.65)
    {
        return ctrl_sevromid+0.65;
    }
    /**小于最小值则输出最小值**/
    else if(sevro < ctrl_sevromid-0.65)
    {
        return ctrl_sevromid-0.65;
    }
    return sevro;  ///< 未超限则输出原值
}

//以下代码速度闭环后再启用
void CTRL_MotorUpdate(float motorL, float motorR)
{
    /**弯道减速**/
    /*if(abs(ctrl_dirPidOutput - ctrl_sevromid) > 0.15)
    {
        motorR -= 5;
        motorL -= 5;
    }*/
    /** 左电机满载 **/
    if(motorL > 50.0f)
    {
        motorL = 50.0f;
    }
    if(motorL < -50.0f)
    {
        motorL = -50.0f;
    }
    /** 右电机满载 **/
    if(motorR > 50.0f)
    {
        motorR = 50.0f;
    }
    if(motorR < -50.0f)
    {
        motorR = -50.0f;
    }
    /** 反转保护 **/

    if(motorL > 0)
    {
        SCFTM_PWM_ChangeHiRes(FTM0, kFTM_Chnl_0, 20000U, motorL);
        SCFTM_PWM_ChangeHiRes(FTM0, kFTM_Chnl_1, 20000U, 0.0f);
    }
    else
    {
        SCFTM_PWM_ChangeHiRes(FTM0, kFTM_Chnl_0, 20000U, 0.0f);
        SCFTM_PWM_ChangeHiRes(FTM0, kFTM_Chnl_1, 20000U, -motorL);
    }

    if(motorR > 0)
    {
        SCFTM_PWM_ChangeHiRes(FTM0, kFTM_Chnl_2, 20000U, motorR);
        SCFTM_PWM_ChangeHiRes(FTM0, kFTM_Chnl_3, 20000U, 0.0f);
    }
    else
    {
        SCFTM_PWM_ChangeHiRes(FTM0, kFTM_Chnl_2, 20000U, 0.0f);
        SCFTM_PWM_ChangeHiRes(FTM0, kFTM_Chnl_3, 20000U, -motorR);
    }
}

float PIDCTRL_DeltaPIGain(pidCtrl_t *_pid)
{
    return (_pid->errCurr-_pid->errPrev)*_pid->kp + _pid->errCurr*_pid->ki;
}
