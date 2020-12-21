#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/soc/qcom/smem.h>
#include <linux/seq_file.h>
#include <soc/oppo/oppo_project.h>
#include <soc/qcom/watchdog.h>
#include <linux/io.h>
#include <linux/input.h>
#include <linux/of.h>
#include <linux/of_fdt.h>

#define TAG	"dump:"


__attribute__((weak)) void oppo_switch_fulldump(bool open)
{
	return;
}

void oppo_force_to_fulldump(bool force)
{
	return;
}


void oppo_sync_key_event(unsigned int type, unsigned int code, int value)
{
	return;
}

