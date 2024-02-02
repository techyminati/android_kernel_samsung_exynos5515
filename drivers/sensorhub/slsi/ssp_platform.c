
#include <linux/kernel.h>
#include <linux/rtc.h>
#include "ssp.h"
#include "ssp_dump.h"
#include "../../staging/nanohub/chub.h"
#include "../../staging/nanohub/chub_dbg.h"

int sensorhub_comms_read(void *ssp_data, u8 *buf, int length, int timeout)
{
	struct contexthub_ipc_info *ipc = ((struct ssp_data *)ssp_data)->platform_data;

	return contexthub_ipc_read(ipc, buf, length, timeout);
}

int sensorhub_comms_write(void *ssp_data, u8 *buf, int length, int timeout)
{
	struct contexthub_ipc_info *ipc = ((struct ssp_data *)ssp_data)->platform_data;

	return contexthub_ipc_write(ipc, buf, length, timeout);
}

int sensorhub_reset(void *ssp_data)
{
	struct contexthub_ipc_info *ipc = ((struct ssp_data *)ssp_data)->platform_data;

	return contexthub_reset(ipc, 1, CHUB_ERR_SSP_RECOVERY);
}

int sensorhub_firmware_download(void *ssp_data)
{
	struct contexthub_ipc_info *ipc = ((struct ssp_data *)ssp_data)->platform_data;

	return contexthub_reset(ipc, 1, 0);
}

int sensorhub_power_on(void *ssp_data)
{
	struct contexthub_ipc_info *ipc = ((struct ssp_data *)ssp_data)->platform_data;

	return contexthub_poweron(ipc);
}

int sensorhub_shutdown(void *ssp_data)
{
#if 0
	struct contexthub_ipc_info *ipc = ((struct ssp_data *)ssp_data)->platform_data;
	int ret;

	ret = contexthub_ipc_write_event(ipc, MAILBOX_EVT_SHUTDOWN);
	if (ret)
		pr_err("shutdonw fails, ret:%d\n", ret);

	return ret;
#else
	return 0;
#endif
}

void *ssp_device_probe(struct device *dev)
{
	return ssp_probe(dev);
}

#ifdef CONFIG_SSP_SLEEP
void ssp_device_remove(void *ssp_data)
{
	struct ssp_data *data = ssp_data;

	ssp_remove(data);
}
#endif

void ssp_device_suspend(void *ssp_data, struct device *dev)
{
	struct ssp_data *data = ssp_data;

	ssp_suspend(data, dev);
}

void ssp_device_resume(void *ssp_data, struct device *dev)
{
	struct ssp_data *data = ssp_data;

	ssp_resume(data, dev);
}

void ssp_platform_init(void *ssp_data, void *platform_data)
{
	struct ssp_data *data = ssp_data;

	data->platform_data = platform_data;
}

void ssp_handle_recv_packet(void *ssp_data, char *packet, int packet_len)
{
	struct ssp_data *data = ssp_data;

	handle_packet(data, packet, packet_len);
}

void ssp_platform_start_refrsh_task(void *ssp_data)
{
	struct ssp_data *data = ssp_data;

	queue_refresh_task(data, 0);
}

enum {
	DUMP_TYPE_INFO,
	DUMP_TYPE_DRAM,
	DUMP_TYPE_SRAM,
	DUMP_TYPE_MAX,
};

#define DUMP_FILES                                                       \
	{                                                                    \
		"SensorHubDump_Info", "SensorHubDump_Dram", "SensorHubDump_Sram" \
	}
#define DUMP_FILE_TYPES       \
	{                         \
		"txt", "dump", "dump" \
	}

int sensorhub_dump_get_num_dumptype(void *ssp_data)
{
	return DUMP_TYPE_MAX;
}

void sensorhub_dump_get_filename(void *ssp_data, int index, char *file_name, char *file_type)
{
	char ary_filename[DUMP_TYPE_MAX][30] = DUMP_FILES;
	char ary_filetype[DUMP_TYPE_MAX][5] = DUMP_FILE_TYPES;

	snprintf(file_name, sizeof(ary_filename[index]), ary_filename[index]);
	snprintf(file_type, sizeof(ary_filetype[index]), ary_filetype[index]);
}

int get_sensorhub_dump_size(void)
{
	return ipc_get_chub_mem_size();
}

void *get_sensorhub_sram_ptr(void)
{
    return ipc_get_base(IPC_REG_DUMP);
}

void ssp_dump_write_file(void *ssp_data, int sec_time, int reason, void *sram_buf, int sram_size, u32 *gpr, int num_gpr)
{
	struct ssp_data *data = ssp_data;
	char dump_info[20] = {0,};

#if 0 //#ifdef CONFIG_SENSORS_SSP_DEBUG
	if (data->is_reset_started == false) {
		ssp_errf("skip ssp dump in case of chub reset panic\n");
		return;
	}
#endif
#ifdef CONFIG_SENSORS_SSP_AP_WAKEUP
	ssp_enable_irq(data, 0);
#endif

	snprintf(dump_info, sizeof(dump_info), "%06u-%02u", sec_time, reason);

	write_ssp_dump_file(data, dump_info, sram_buf, sram_size, reason, gpr, num_gpr);

	if (reason < CHUB_ERR_MAX) {
		ssp_errf("reason %d, data->reset_type %d info %s\n", reason, data->reset_type, dump_info);
		data->reset_type = RESET_TYPE_CHUB_CRASHED;
		//sensorhub_reset(data);
	}
}

bool is_sensorhub_working(void *ssp_data)
{
	struct contexthub_ipc_info *ipc = ((struct ssp_data *)ssp_data)->platform_data;

	if (atomic_read(&ipc->chub_status) == CHUB_ST_RUN && atomic_read(&ipc->in_reset) == 0) {
		return true;
	} else {
		pr_err("[SSP] shutdown reason:%d:%d\n", atomic_read(&ipc->chub_status), atomic_read(&ipc->in_reset));
		return false;
	}
}

int ssp_download_firmware(void *ssp_data, struct device *dev, void *addr)
{
	struct ssp_data *data = ssp_data;
	return download_sensorhub_firmware(data, dev, addr);
}

void ssp_set_firmware_type(void *ssp_data, int type)
{
	struct ssp_data *data = ssp_data;
	data->fw_type = type;

	ssp_infof("%s : %d", __func__, data->fw_type);
}

int ssp_get_firmware_type(void *ssp_data)
{
	struct ssp_data *data = ssp_data;

	ssp_infof("%s : %d", __func__, data->fw_type);
	return data->fw_type;
}

void ssp_chub_bin_logbuf_dram_read(void *ssp_data, char *buf)
{
	struct contexthub_ipc_info *chub = ((struct ssp_data *)ssp_data)->platform_data;

	memcpy(buf, chub->chub_rt_log.buffer, chub->chub_rt_log.buffer_size);
}

char *ssp_get_chub_bin_logbuf_ptr(void *ssp_data)
{
	struct contexthub_ipc_info *chub = ((struct ssp_data *)ssp_data)->platform_data;

	return chub->chub_rt_log.buffer;
}

unsigned int ssp_get_chub_bin_logbuf_size(void *ssp_data)
{
	struct contexthub_ipc_info *chub = ((struct ssp_data *)ssp_data)->platform_data;

	return chub->chub_rt_log.buffer_size;
}

unsigned int ssp_get_logbuf_write_index(void *ssp_data)
{
	struct contexthub_ipc_info *chub = ((struct ssp_data *)ssp_data)->platform_data;

	return chub->chub_rt_log.write_index;
}


void ssp_device_enable_ap_wakeup(void *ssp_data, struct device *dev)
{
#ifdef CONFIG_SENSORS_SSP_AP_WAKEUP
	struct ssp_data *data = ssp_data;

	if (data->irq_init_done == true) {
		ssp_enable_ap_wakeup(data, dev);
	}
#endif
}

int ssp_check_hrm_int(void *ssp_data, struct device *dev)
{
#if defined(CONFIG_SENSORS_SSP_WISE) || defined(CONFIG_SENSORS_SSP_FRESH) || defined(CONFIG_SENSORS_SSP_HEART)
	struct ssp_data *data = ssp_data;
	return check_hrm_vendor(data, dev);
#else
	return 0;
#endif
}

