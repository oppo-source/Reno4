/***************************************************
 * File:touch.c
 * VENDOR_EDIT
 * Copyright (c)  2008- 2030  Oppo Mobile communication Corp.ltd.
 * Description:
 *             tp dev
 * Version:1.0:
 * Date created:2016/09/02
 * Author: hao.wang@Bsp.Driver
 * TAG: BSP.TP.Init
*/

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/serio.h>
#include "oppo_touchscreen/tp_devices.h"
#include "oppo_touchscreen/touchpanel_common.h"
#include <soc/oppo/oppo_project.h>
#include <soc/oppo/device_info.h>

#define MAX_LIMIT_DATA_LENGTH         100

extern char *saved_command_line;
int g_tp_dev_vendor = TP_UNKNOWN;
int g_tp_prj_id = 0;

/*if can not compile success, please update vendor/oppo_touchsreen*/
struct tp_dev_name tp_dev_names[] = {
    {TP_OFILM, "OFILM"},
    {TP_BIEL, "BIEL"},
    {TP_TRULY, "TRULY"},
    {TP_BOE, "BOE"},
    {TP_G2Y, "G2Y"},
    {TP_TPK, "TPK"},
    {TP_JDI, "JDI"},
    {TP_TIANMA, "TIANMA"},
    {TP_SAMSUNG, "SAMSUNG"},
    {TP_DSJM, "DSJM"},
    {TP_BOE_B8, "BOEB8"},
    {TP_UNKNOWN, "UNKNOWN"},
};

#define GET_TP_DEV_NAME(tp_type) ((tp_dev_names[tp_type].type == (tp_type))?tp_dev_names[tp_type].name:"UNMATCH")

bool __init tp_judge_ic_match(char *tp_ic_name)
{
    pr_err("[TP] tp_ic_name = %s \n", tp_ic_name);
    //pr_err("[TP] boot_command_line = %s \n", saved_command_line);

    switch(get_project()) {
    case 19365:
    case 19367:
    case 19368:

        g_tp_prj_id = 19365;
        if (strstr(tp_ic_name, "Goodix-gt9886") && (strstr(saved_command_line, "dsi_oppo19365samsung_ams644vk01_1080_2400_cmd_display")
                        || strstr(saved_command_line, "dsi_oppo19365samsung_ams643xf01_1080_2400_cmd_display"))) {
            g_tp_dev_vendor = TP_SAMSUNG;
            return true;
        }
        pr_err("[TP] Driver does not match the project\n");
        break;

    case 19567:
    case 19568:
    case 19569:

        g_tp_prj_id = 19567;
        if(strstr(tp_ic_name, "sec-s6sy771") && strstr(saved_command_line, "dsi_oppo19567samsung_amb655uv01_1080_2400_cmd_display")) {
            g_tp_dev_vendor = TP_SAMSUNG;
            return true;
        }
        pr_err("[TP] Driver does not match the project\n");
        break;

    default:
        pr_err("Invalid project\n");
        break;
    }

    pr_err("Lcd module not found\n");

    return false;
}

int tp_util_get_vendor(struct hw_resource *hw_res, struct panel_info *panel_data)
{
    char* vendor;

    panel_data->test_limit_name = kzalloc(MAX_LIMIT_DATA_LENGTH, GFP_KERNEL);
    if (panel_data->test_limit_name == NULL) {
        pr_err("[TP]panel_data.test_limit_name kzalloc error\n");
    }

   panel_data->tp_type = g_tp_dev_vendor;

    if (panel_data->tp_type == TP_UNKNOWN) {
        pr_err("[TP]%s type is unknown\n", __func__);
        return 0;
    }

    vendor = GET_TP_DEV_NAME(panel_data->tp_type);

    strcpy(panel_data->manufacture_info.manufacture, vendor);

    snprintf(panel_data->fw_name, MAX_FW_NAME_LENGTH,
            "tp/%d/FW_%s_%s.img",
            g_tp_prj_id, panel_data->chip_name, vendor);

    if (g_tp_prj_id == 19365 && (strstr(saved_command_line, "dsi_oppo19365samsung_ams643xf01_1080_2400_cmd_display"))) {
            snprintf(panel_data->fw_name, MAX_FW_NAME_LENGTH,
            "tp/%d/FW_%s_%s_NEW.img",
            g_tp_prj_id, panel_data->chip_name, vendor);
    }

    if (panel_data->test_limit_name) {
        snprintf(panel_data->test_limit_name, MAX_LIMIT_DATA_LENGTH,
            "tp/%d/LIMIT_%s_%s.img",
            g_tp_prj_id, panel_data->chip_name, vendor);
    }

    switch(get_project()) {
    case 19365:
    case 19367:
    case 19368:

        memcpy(panel_data->manufacture_info.version, "0xbd3320000", 11);
        break;
    case 19567:
    case 19568:
    case 19569:

        if(strstr(saved_command_line, "dsi_oppo19567samsung_amb655uv01_1080_2400_cmd_display")) {
            memcpy(panel_data->manufacture_info.version, "0xad1220000", 11);
        }
        break;
    }

    panel_data->manufacture_info.fw_path = panel_data->fw_name;

    pr_info("[TP]vendor:%s fw:%s limit:%s\n",
        vendor,
        panel_data->fw_name,
        panel_data->test_limit_name == NULL ? "NO Limit" : panel_data->test_limit_name);

    return 0;
}

