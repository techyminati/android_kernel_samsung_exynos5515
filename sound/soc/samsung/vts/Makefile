snd-soc-samsung-vts-mailbox-$(CONFIG_SND_SOC_SAMSUNG_MAILBOX) += mailbox.o vts_irq.o
ifeq ($(SND_SOC_SAMSUNG_VTS_S_LIF), yes)
snd-soc-samsung-vts-$(CONFIG_SND_SOC_SAMSUNG_VTS) += vts.o \
	vts_log.o vts_dump.o vts_dbg.o vts_dma.o \
	vts_util.o \
	vts_s_lif_clk_table.o vts_s_lif_nm.o vts_s_lif_soc.o vts_s_lif.o \
	vts_s_lif_dump.o \
	vts_res.o vts_pcm_dump.o
else
snd-soc-samsung-vts-$(CONFIG_SND_SOC_SAMSUNG_VTS) += vts.o \
        vts_log.o vts_dump.o vts_dbg.o vts_dma.o \
        vts_util.o vts_res.o vts_pcm_dump.o vts_proc.o
endif
obj-$(CONFIG_SND_SOC_SAMSUNG_VTS) += snd-soc-samsung-vts-mailbox.o snd-soc-samsung-vts.o
