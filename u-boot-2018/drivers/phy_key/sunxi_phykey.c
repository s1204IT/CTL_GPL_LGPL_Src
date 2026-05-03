/* PDX-License-Identifier:	GPL-2.0+ */

#include <common.h>
#include <sys_config.h>
#include <physical_key.h>
#include <fdt_support.h>

#define FDT_ATTRIBUTE_KEY_TEPY	"key_type"

__attribute__((section(".data"))) static struct sunxi_phykey_ops *phykey_ops;

static struct sunxi_phykey_ops *get_phykey_ops(void)
{
	int nodeoffset, ret = 0;
	char *key_type = NULL;
	struct sunxi_phykey_ops *sunxi_phykey_temp;
	struct sunxi_phykey_ops *sunxi_phykey_start =
		ll_entry_start(struct sunxi_phykey_ops, phykey);
	int max = ll_entry_count(struct sunxi_phykey_ops, phykey);

	nodeoffset = fdt_path_offset(working_fdt, FDT_PATH_KEY_DETECT);
	if (nodeoffset > 0)
		ret = fdt_getprop_string(working_fdt, nodeoffset, FDT_ATTRIBUTE_KEY_TEPY, &key_type);

	if ((sunxi_phykey_start == NULL) || ret < 0)
		return NULL;

	for (sunxi_phykey_temp = sunxi_phykey_start;
	     sunxi_phykey_temp != sunxi_phykey_start + max;
	     sunxi_phykey_temp++) {
	    if (!strcmp(key_type, sunxi_phykey_temp->phykey_name))
				return sunxi_phykey_temp;
	}
	pr_err("phykey: no found\n");
	return NULL;
}

int sunxi_phykey_probe(void)
{
	phykey_ops = get_phykey_ops();

	if (phykey_ops == NULL)
		return -1;
	return 0;
}

int sunxi_phykey_init(void)
{
	if ((phykey_ops) && (phykey_ops->phykey_init))
		return phykey_ops->phykey_init();
	return -1;
}

int sunxi_phykey_read(void)
{
	if ((phykey_ops) && (phykey_ops->phykey_read))
		return phykey_ops->phykey_read();
	return -1;
}

int sunxi_phykey_exit(void)
{
	if ((phykey_ops) && (phykey_ops->phykey_exit))
		return phykey_ops->phykey_exit();
	return -1;
}
