# SPDX-License-Identifier: GPL-2.0

dtb-y += k230-canmv-01studio-spidev0_0.dtbo
dtb-y += k230-canmv-01studio-spidev0_1.dtbo
dtb-y += k230-canmv-01studio-csi0.dtbo
dtb-y += k230-canmv-01studio-csi1.dtbo
dtb-y += k230-canmv-01studio-csi2.dtbo
dtb-y += k230-xcam-csi2.dtbo

$(obj)/%.dtbo: $(src)/%.dts FORCE
	$(call if_changed_dep,dtc)

clean-files := *.dtbo