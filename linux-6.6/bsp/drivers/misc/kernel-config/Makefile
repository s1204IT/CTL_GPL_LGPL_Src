# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the linux kernel.
#

ifeq ($(CONFIG_IKCONFIG),y)

$(obj)/aw-config_data.gz: $(objtree)/.config FORCE
	@echo " " > $(shell dirname $@)/.$(shell basename $@).cmd
	@cat $< | gzip - > $@

$(obj)/aw-config_data.c: $(obj)/aw-config_data.gz FORCE
	@echo " " > $(shell dirname $@)/.$(shell basename $@).cmd
	@echo '#include <linux/kernel.h>'  > $@
	@echo '#include <linux/module.h>' >> $@
	@xxd  -i -c 16  $<                >> $@
	@echo 'MODULE_LICENSE("GPL");'    >> $@
	@sed  -i 's/unsigned char .*=/unsigned char aw_config_data[] =/g' $@
	@sed  -i 's/unsigned int .*=/unsigned int aw_config_data_len =/g' $@

aw-config-y := configs.o aw-config_data.o
obj-m += aw-config.o

endif
