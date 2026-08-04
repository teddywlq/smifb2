#include "ddk768_help.h"
#include <linux/pci.h>

volatile unsigned char __iomem * mmio768 = NULL;
struct pci_dev *g_pdev = NULL;

/* after driver mapped io registers, use this function first */
void ddk768_set_mmio(struct pci_dev *dev, volatile unsigned char * addr)
{
	mmio768 = addr;
	g_pdev = dev;
	printk("Found SM768 SOC Chip\n");
}

#if SM768_REG_EXTERNAL

unsigned int peekRegisterDWord_External(unsigned int addr)
{
	unsigned int value;

	pci_write_config_dword(g_pdev, 0x0, 0x0);
	value = readl(addr + mmio768);
	pci_write_config_dword(g_pdev, 0x0, 0x0);

	return value;
}

void pokeRegisterDWord_External(unsigned int addr, unsigned int data)
{
	pci_write_config_dword(g_pdev, 0x0, 0x0);
	writel(data, addr + mmio768);
	pci_write_config_dword(g_pdev, 0x0, 0x0);
}

#endif
