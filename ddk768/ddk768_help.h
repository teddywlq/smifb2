#ifndef _DDK768_HELP_H__
#define _DDK768_HELP_H__

#ifndef USE_INTERNAL_REGISTER_ACCESS

#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include "../smi_drv.h"

struct pci_dev;

#if SM768_REG_EXTERNAL

unsigned int peekRegisterDWord_External(unsigned int addr);
void pokeRegisterDWord_External(unsigned int addr, unsigned int data);

#define peekRegisterDWord(addr) peekRegisterDWord_External(addr)
#define pokeRegisterDWord(addr, data) pokeRegisterDWord_External((addr), (data))

#else

#define peekRegisterDWord(addr) readl((addr) + mmio768)
#define pokeRegisterDWord(addr, data) writel((data), (addr) + mmio768)

#endif

#define peekRegisterByte(addr) readb((addr)+mmio768)
#define pokeRegisterByte(addr,data) writeb((data),(addr)+mmio768)


/* Size of SM768 MMIO and memory */
#define SM768_PCI_ALLOC_MMIO_SIZE       (2*1024*1024)
#define SM768_PCI_ALLOC_MEMORY_SIZE     (128*1024*1024)


void ddk768_set_mmio(struct pci_dev *dev, volatile unsigned char * addr);

extern volatile unsigned  char __iomem * mmio768;

#else
/* implement if you want use it*/
#endif

#endif
