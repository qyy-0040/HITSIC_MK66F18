#ifndef DEV_CONTROL_HPP_
#define DEV_CONTROL_HPP_
#include "hitsic_common.h"
#include "inc_stdlib.hpp"

#include "sys_pitmgr.hpp"
#include "sys_extint.hpp"
#include "drv_imu_invensense.hpp"
#include "lib_pidctrl.h"

#include "app_menu.hpp"
#include "sc_ftm.h"
#include "drv_disp_ssd1306_port.hpp"

#include "dev_em.hpp"
#include "dev_Image.h"

#define CTRL_SPD_CTRL_MS    (5U)
#define CTRL_DIR_CTRL_MS    (20U)
#define CTRL_EM_CTRL_MS     (20U)
#define CTRL_STR_CTRL_MS     (20U)
#define CTRL_MODE_IMG    (0U)
#define CTRL_MODE_EMA    (1U)

#define CTRL_1G             (9.80f)
//#define CTRL_ASIN(x)        (arm_arcsin_f32(x))
#define CTRL_ASIN(x)        (asin(x))

#define CTRL_PI             (3.1415926f)

#define CTRL_DEG2RAD(x)     (x * (CTRL_PI / 180.0f))
#define CTRL_RAD2DEG(x)     (x * (180.0f / CTRL_PI))

#define CTRL_ENCO_SPD_COEFF (0.034778)

extern inv::mpu6050_t imu_6050;

void CTRL_Init(void);

void CTRL_MenuInit(menu_list_t *menuList);

/* ******************** 启动延时 ******************** */
void CTRL_StrCtrl(void *userData);
void CTRL_Start(void);

/* *********************************************** */

/* ******************** 速度环 ******************** */
extern int32_t ctrl_spdCtrlEn[3];
extern float ctrl_motorL[2];
extern float ctrl_motorR[2];
extern float ctrl_spdSet;
extern pidCtrl_t ctrl_spdPid[2];
extern float ctrl_spdL, ctrl_spdR;
extern float ctrl_spdLerror ,ctrl_spdRerror;
extern float ctrl_spdLOutput;
extern float ctrl_spdROutput;

void CTRL_SpdtestCtrl(void *userData);
bool CTRL_Protect(int32_t ctrl_mode);
void CTRL_SpdCtrl(void *userData);

/* *********************************************** */


/* ******************** 转向环 ******************** */
extern int32_t ctrl_dirCtrlEn[3];
extern int32_t ctrl_modesel[3];
extern int32_t ctrl_autoselEn[3];
extern pidCtrl_t ctrl_dirPid[2];
extern float ctrl_dirPidOutput;
extern float ctrl_errtor;
extern int32_t ctrl_mode;

int32_t CTRL_GetCtrlMode(void);
float CTRL_GetDirError(int ctrl_dirmode);
void CTRL_CheckZebra(void);
void CTRL_DirCtrl(void *userData);
float CTRL_SevroUpdate(float sevro);

/* *********************************************** */



void CTRL_MotorUpdate(float motorL, float motorR);
float PIDCTRL_DeltaPIGain(pidCtrl_t *_pid);


#endif /* DEV_CONTROL_HPP_ */
