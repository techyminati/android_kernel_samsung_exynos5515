#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/pm_qos.h>
#include <linux/workqueue.h>
#include <linux/printk.h>

#include <linux/cpufreq_pmqos_boost.h>

#define WRISTUP_CPU_MIN_FREQ    1200000 /* 1.2 GHz */
#define WRISTUP_PERIOD_US       500000  /* 500 ms */

static int wristup_initialized = 0;

/* pmqos request */
static struct pm_qos_request wristup_qos_request;

/* interface for wrist-up boost */
void wristup_booster_turn_on(void)
{
    if (wristup_initialized) {
        pm_qos_update_request_timeout(
            &wristup_qos_request, WRISTUP_CPU_MIN_FREQ, WRISTUP_PERIOD_US);
    }
}

static int __init cpufreq_pmqos_init(void)
{
    wristup_initialized = 1;

    pm_qos_add_request(&wristup_qos_request, PM_QOS_CLUSTER0_FREQ_MIN, 0);

    return 0;
}
late_initcall(cpufreq_pmqos_init);
