// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2024 Embedded Labworks
 *
 * Author: Sergio Prado <sergio.prado@e-labworks.com>
 */

#include <spl.h>
#include <init.h>
#include <fsl_esdhc_imx.h>
#include <asm/io.h>
#include <asm/sections.h>
#include <asm/arch-mx7/clock.h>
#include <asm/arch/mx7-pins.h>
#include <asm/arch-mx7/mx7-ddr.h>

#if defined(CONFIG_SPL_BUILD)

static struct ddrc ddrc_regs_val = {
	.mstr		= 0x01040001,
	.init1		= 0x00690000,
	.init0		= 0x00020083,
	.init3		= 0x09300004,
	.init4		= 0x04480000,
	.init5		= 0x00100004,
	.rankctl	= 0x0000033f,
	.dramtmg0	= 0x0910090a,
	.dramtmg1	= 0x000d020e,
	.dramtmg2	= 0x03040307,
	.dramtmg3	= 0x00002006,
	.dramtmg4	= 0x04020204,
	.dramtmg5	= 0x03030202,
	.dramtmg8	= 0x00000803,
	.zqctl0		= 0x00800020,
	.zqctl1		= 0x02001000,
	.dfitmg0	= 0x02098204,
	.dfitmg1	= 0x00030303,
	.dfiupd0	= 0x80400003,
	.dfiupd1	= 0x00100020,
	.dfiupd2	= 0x80100004,
	.odtcfg		= 0x06000601,
	.odtmap		= 0x00000001,
	.rfshtmg	= 0x00400046,
	.addrmap0	= 0x0000001f,
	.addrmap1	= 0x00080808,
	.addrmap5	= 0x07070707,
	.addrmap6	= 0x07070707,
};

static struct ddrc_mp ddrc_mp_val = {
	.pctrl_0	= 0x00000001,
};

static struct ddr_phy ddr_phy_regs_val = {
	.phy_con0	= 0x17420f40,
	.phy_con1	= 0x10210100,
	.phy_con4	= 0x00060807,
	.mdll_con0	= 0x1010007e,
	.drvds_con0	= 0x00000d6e,
	.cmd_sdll_con0	= 0x00000010,
	.offset_lp_con0	= 0x0000000f,
	.offset_rd_con0	= 0x08080808,
	.offset_wr_con0	= 0x08080808,
};

static struct mx7_calibration calib_param = {
	.num_val	= 5,
	.values		= {
		0x0e407304,
		0x0e447304,
		0x0e447306,
		0x0e447304,
		0x0e407304,
	},
};

static void ddr_init(void)
{
	mx7_dram_cfg(&ddrc_regs_val, &ddrc_mp_val, &ddr_phy_regs_val,
		&calib_param);
}

static void gpr_init(void)
{
	struct iomuxc_gpr_base_regs *gpr_regs =
		(struct iomuxc_gpr_base_regs *)IOMUXC_GPR_BASE_ADDR;

	writel(0x4F400005, &gpr_regs->gpr[1]);
	writel(0x00000178, &gpr_regs->gpr[8]);
}

void board_init_f(ulong dummy)
{
	arch_cpu_init();
	gpr_init();
	board_early_init_f();
	timer_init();
	preloader_console_init();
	ddr_init();
	memset(__bss_start, 0, __bss_end - __bss_start);
#ifdef CONFIG_TARGET_COLIBRI_IMX7_NAND
	setup_gpmi_nand();
#endif
	board_init_r(NULL, 0);
}

#define USDHC_PAD_CTRL	(PAD_CTL_DSE_3P3V_32OHM | PAD_CTL_SRE_SLOW | \
				PAD_CTL_HYS | PAD_CTL_PUE | PAD_CTL_PUS_PU47KOHM)

static iomux_v3_cfg_t const usdhc3_emmc_pads[] = {
	MX7D_PAD_SD3_CLK__SD3_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_CMD__SD3_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA0__SD3_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA1__SD3_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA2__SD3_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA3__SD3_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA4__SD3_DATA4 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA5__SD3_DATA5 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA6__SD3_DATA6 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA7__SD3_DATA7 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_STROBE__SD3_STROBE	 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
};

int board_mmc_init(struct bd_info *bis)
{
	static struct fsl_esdhc_cfg usdhc_cfg;
	int ret = 0;

	imx_iomux_v3_setup_multiple_pads(usdhc3_emmc_pads,
					 ARRAY_SIZE(usdhc3_emmc_pads));

	usdhc_cfg.esdhc_base = USDHC3_BASE_ADDR;
	usdhc_cfg.sdhc_clk = mxc_get_clock(MXC_ESDHC3_CLK);
	usdhc_cfg.max_bus_width = 8;

	ret = fsl_esdhc_initialize(bis, &usdhc_cfg);
	if (ret)
		log_err("Error: could not initialize MMC. (%d)\n", ret);

	return ret;
}

int board_mmc_getcd(struct mmc *mmc)
{
	return 1; /* assume uSDHC3 emmc is always present */
}

#endif /* CONFIG_SPL_BUILD */
