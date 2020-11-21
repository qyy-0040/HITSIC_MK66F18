#include "dev_control.hpp"

/**控制环PITMGR任务句柄**/
pitMgr_t* ctrl_spdCtrlHandle = nullptr;
pitMgr_t* ctrl_dirCtrlHandle = nullptr;
pitMgr_t* ctrl_emCtrlHandle = nullptr;

/**控制环初始化**/
void CTRL_Init(void)
{
    ctrl_dirCtrlHandle = pitMgr_t::insert(CTRL_DIR_CTRL_MS, 3U, CTRL_DirCtrl, pitMgr_t::enable);
    assert(ctrl_dirCtrlHandle);
    ctrl_spdCtrlHandle = pitMgr_t::insert(CTRL_SPD_CTRL_MS, 4U, CTRL_SpdCtrl, pitMgr_t::enable);
    assert(ctrl_spdCtrlHandle);
    ctrl_emCtrlHandle = pitMgr_t::insert(CTRL_EM_CTRL_MS, 5U, EM_ErrorUpdate, pitMgr_t::enable);
    assert(ctrl_emCtrlHandle);
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

/**全局变量声明**/
float ctrl_sevromid;                ///< 舵机中值
bool ctrl_startstaus = false;       ///< 启动状态
int32_t ctrl_testEn[3] = {0, 0, 1}; ///< 测试模式使能

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
        MENU_ListInsert(settingMenuList, MENU_ItemConstruct(procType, CTRL_Start, "start/stop", 0U,
                menuItem_proc_runOnce));
        MENU_ListInsert(settingMenuList, MENU_ItemConstruct(variType, &ctrl_testEn[0], "mode.test", 0U,
                menuItem_data_NoSave | menuItem_data_NoLoad | menuItem_dataExt_HasMinMax));
        MENU_ListInsert(settingMenuList, MENU_ItemConstruct(variType, &ctrl_modesel[0], "mode.sel", 0U,
                menuItem_data_NoSave | menuItem_data_NoLoad | menuItem_dataExt_HasMinMax));
        /*MENU_ListInsert(ctrlMenuList, MENU_ItemConstruct(variType, &ctrl_autoselEn[0], "mode.auto", 0U,
                menuItem_data_NoSave | menuItem_data_NoLoad | menuItem_dataExt_HasMinMax));
        MENU_ListInsert(ctrlMenuList, MENU_ItemConstruct(varfType, &ctrl_errtor, "errtor", 11U,
                menuItem_data_region));*/
        MENU_ListInsert(settingMenuList, MENU_ItemConstruct(variType, &ctrl_spdCtrlEn[0], "spd.en", 0U,
                menuItem_data_NoSave | menuItem_data_NoLoad | menuItem_dataExt_HasMinMax));
        MENU_ListInsert(settingMenuList, MENU_ItemConstruct(variType, &ctrl_dirCtrlEn[0], "dir.en", 0U,
                menuItem_data_NoSave | menuItem_data_NoLoad | menuItem_dataExt_HasMinMax));

    }
    MENU_ListInsert(menuList, MENU_ItemConstruct(menuType, ctrlMenuList, "Control", 0, 0));
    {
        MENU_ListInsert(ctrlMenuList, MENU_ItemConstruct(nullType, NULL, "DIRECTION", 0, 0));
        MENU_ListInsert(ctrlMenuList, MENU_ItemConstruct(varfType, &ctrl_sevromid, "sevro.mid", 20U,
                menuItem_data_region));
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
        MENU_ListInsert(ctrlMenuList, MENU_ItemConstruct(varfType, &ctrl_motorL[0], "MotorL.img", 12U,
                menuItem_data_region));
        MENU_ListInsert(ctrlMenuList, MENU_ItemConstruct(varfType, &ctrl_motorR[0], "MotorR.img", 13U,
                menuItem_data_region)); //速度闭环后就删掉
        MENU_ListInsert(ctrlMenuList, MENU_ItemConstruct(varfType, &ctrl_motorL[1], "MotorL.ema", 14U,
                menuItem_data_region));
        MENU_ListInsert(ctrlMenuList, MENU_ItemConstruct(varfType, &ctrl_motorR[1], "MotorR.ema", 15U,
                menuItem_data_region));
        /*MENU_ListInsert(ctrlMenuList, MENU_ItemConstruct(varfType, &ctrl_spdSet, "spdSet", 11U,
                menuItem_data_region));
        MENU_ListInsert(ctrlMenuList, MENU_ItemConstruct(varfType, &ctrl_spdPid.kp, "spd.kp", 13U,
                menuItem_data_region));
        MENU_ListInsert(ctrlMenuList, MENU_ItemConstruct(varfType, &ctrl_spdPid.ki, "spd.ki", 14U,
                menuItem_data_region));
        MENU_ListInsert(ctrlMenuList, MENU_ItemConstruct(varfType, &ctrl_spdPid.kd, "spd.kd", 15U,
                menuItem_data_region));
        MENU_ListInsert(ctrlMenuList, MENU_ItemConstruct(varfType, &ctrl_spdPidOutput, "spd.out", 0U,
                menuItem_data_NoSave | menuItem_data_NoLoad));速度闭环后启用*/
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
        MENU_ListInsert(paraMenuList, MENU_ItemConstruct(varfType, &img_error, "img.error", 0U,
                menuItem_data_NoSave | menuItem_data_NoLoad|menuItem_data_ROFlag));
        MENU_ListInsert(paraMenuList, MENU_ItemConstruct(varfType, &ema_error, "ema.error", 0U,
                menuItem_data_NoSave | menuItem_data_NoLoad|menuItem_data_ROFlag));
        MENU_ListInsert(paraMenuList, MENU_ItemConstruct(varfType, &ctrl_dirPidOutput, "servo.out", 0U,
                menuItem_data_NoSave | menuItem_data_NoLoad|menuItem_data_ROFlag));

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
void CTRL_Start(void)
{
    /**未启动且不在调试模式时延时启动**/
    if(ctrl_startstaus == false && 0 == ctrl_testEn[0])
    {
        DISP_SSD1306_delay_ms(1500);///< 延时
        ctrl_startstaus = !ctrl_startstaus;
    }
    else
    {
        ctrl_startstaus = !ctrl_startstaus;///< 启动/停止
    }
}

/* *********************************************** */

/* ******************** 速度环 ******************** */
int32_t ctrl_spdCtrlEn[3] = {0, 0, 1}; ///< 速度环使能
float ctrl_motorL[2];
float ctrl_motorR[2];

//以下代码将在速度闭环后使用
/*float ctrl_spdSet = 0.0f; ///< 速度设置

pidCtrl_t ctrl_spdPid =
{
    .kp = 0.0f, .ki = 0.0f, .kd = 0.0f,
    .errCurr = 0.0f, .errIntg = 0.0f, .errDiff = 0.0f, .errPrev = 0.0f,
};

float ctrl_spdL = 0.0f, ctrl_spdR = 0.0f;
float ctrl_spdAvg = 0.0f;

float ctrl_spdPidOutput = 0.0f;*/ ///< 速度环输出

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
    //以下代码将在速度闭环后使用(下同)
    //ctrl_spdL = ((float)SCFTM_GetSpeed(ENCO_L_PERIPHERAL)) * CTRL_ENCO_SPD_COEFF;
    //SCFTM_ClearSpeed(ENCO_L_PERIPHERAL);
    //ctrl_spdR = ((float)SCFTM_GetSpeed(ENCO_R_PERIPHERAL)) * CTRL_ENCO_SPD_COEFF;
    //SCFTM_ClearSpeed(ENCO_R_PERIPHERAL);
    //ctrl_spdAvg = (ctrl_spdL + ctrl_spdR) / 2.0f;
    /**已启动且速度环已使能更新电机输出**/
    if(1 == ctrl_spdCtrlEn[0] && ctrl_startstaus)
    {
        /**出赛道电机停转**/
        if(CTRL_Protect(ctrl_mode))
        {
            CTRL_MotorUpdate(0, 0);
        }
        //PIDCTRL_ErrUpdate(&ctrl_spdPid, ctrl_spdAvg - ctrl_spdSet);
        //ctrl_spdPidOutput = PIDCTRL_CalcPIDGain(&ctrl_spdPid);
        else
        {
            /**按模式选择电机转速**/
            CTRL_MotorUpdate(ctrl_motorL[ctrl_mode], ctrl_motorR[ctrl_mode]);
        }
    }
    /**未启动时电机停转**/
    else
    {
        //ctrl_spdPidOutput = 0.0f;
        CTRL_MotorUpdate(0, 0);
    }
}

/* *********************************************** */

/* ******************** 转向环 ******************** */
int32_t ctrl_dirCtrlEn[3] = {0, 0, 1}; ///< 转向环使能
int32_t ctrl_modesel[3] = {0, 0, 1};   ///< 模式选择
int32_t ctrl_autoselEn[3] = {0, 0, 1}; ///< 自动模式使能
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

void CTRL_DirCtrl(void *userData)   ///< 方向环主函数
{
    /**已启动且方向环已使能更新舵机输出**/
    if(1 == ctrl_dirCtrlEn[0] && ctrl_startstaus)
    {
        CTRL_GetCtrlMode();            ///< 获取当前模式
        ctrl_mode = ctrl_modesel[0];   ///< 更新当前模式
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
    if(abs(ctrl_dirPidOutput - ctrl_sevromid) > 0.15)
    {
        motorR -= 5;
        motorL -= 5;
    }
    /** 左电机满载 **/
    if(motorL > 100.0f)
    {
        motorR -= (motorL - 100.0f);
        motorL = 100.0f;
    }
    if(motorL < -100.0f)
    {
        motorR -= (motorL + 100.0f);
        motorL = -100.0f;
    }
    /** 右电机满载 **/
    if(motorR > 100.0f)
    {
        motorL -= (motorL - 100.0f);
        motorR = 100.0f;
    }
    if(motorR < -100.0f)
    {
        motorL -= (motorL + 100.0f);
        motorR = -100.0f;
    }
    /** 反转保护 **/
    if(motorL < 0.0f && motorR > 0.0f)
    {
        motorL = 0.0f;
    }
    if(motorL > 0.0f && motorR < 0.0f)
    {
        motorR = 0.0f;
    }

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


