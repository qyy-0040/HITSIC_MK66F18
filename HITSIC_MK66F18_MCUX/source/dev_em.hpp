/*
 * dev_em.hpp
 *
 *  Created on: 2020年11月19日
 *      Author: fanyi
 */

#ifndef DEV_EM_HPP_
#define DEV_EM_HPP_

#include "hitsic_common.h"
#include "inc_stdlib.hpp"

#include "sys_pitmgr.hpp"
#include "sys_extint.hpp"
#include "drv_imu_invensense.hpp"
#include "lib_pidctrl.h"

#include "app_menu.hpp"
#include "sc_ftm.h"
#include "sc_adc.h"
#include "drv_disp_ssd1306_port.hpp"

#include "dev_Image.h"


#define SampleTimes 25
#define ChannelTimes 8

void EM_LVSample(void);
void EM_LVGetVal(void);
void swap(uint32_t * a, uint32_t * b);
void EM_ErrorUpdate(void);

#endif /* DEV_EM_HPP_ */
