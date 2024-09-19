ifneq ($(KERNELRELEASE),)

Driver=smifb
obj-m := ${Driver}.o
${Driver}-objs :=smi_drv.o smi_main.o smi_mode.o smi_plane.o smi_ttm.o smi_prime.o hw750.o hw768.o smi_debugfs.o
${Driver}-objs += ddk750/ddk750_help.o  ddk750/ddk750_chip.o  ddk750/ddk750_clock.o  ddk750/ddk750_mode.o ddk750/ddk750_power.o ddk750/ddk750_helper.o ddk750/ddk750_display.o ddk750/ddk750_2d.o ddk750/ddk750_edid.o ddk750/ddk750_swi2c.o ddk750/ddk750_hwi2c.o ddk750/ddk750_cursor.o


ifeq ($(hdmi),1)
EXTRA_CFLAGS += -DUSE_HDMICHIP
${Driver}-y += ddk750/ddk750_sii9022.o
${Driver}-y += ddk750/siHdmiTx_902x_TPI.o
endif

ifeq ($(ep952),1)
EXTRA_CFLAGS += -DUSE_EP952
${Driver}-y += ddk768/ddk768_ep952.o
endif


${Driver}-objs += ddk768/ddk768_help.o  ddk768/ddk768_chip.o  ddk768/ddk768_clock.o  ddk768/ddk768_mode.o ddk768/ddk768_power.o ddk768/ddk768_helper.o ddk768/ddk768_display.o ddk768/ddk768_2d.o \
ddk768/ddk768_edid.o ddk768/ddk768_swi2c.o ddk768/ddk768_hwi2c.o ddk768/ddk768_cursor.o ddk768/ddk768_video.o ddk768/ddk768_hdmi.o ddk768/ddk768_pwm.o ddk768/ddk768_timer.o ddk768/ddk768_intr.o 

ifeq ($(noaudio),1)
EXTRA_CFLAGS += -DNO_AUDIO
else
${Driver}-y += smi_snd.o
${Driver}-y += ddk768/ddk768_iis.o
${Driver}-y += ddk768/uda1345.o
${Driver}-y += ddk768/l3.o
${Driver}-y += ddk768/wm8978.o
endif

ifeq ($(prime),1)
EXTRA_CFLAGS += -DPRIME
endif

ccflags-y :=-O2 -fno-tree-scev-cprop -D_D_SMI -D_D_SMI_D -D__cdecl

else

ifneq ($(RUN_DEPMOD),)
	DEPMOD := /sbin/depmod -a
else
	DEPMOD := true
endif

CONFIG_MODULE_SIG=n
knv :=$(shell uname -r)
KERNELDIR :=/lib/modules/$(knv)/build
PWD := $(shell pwd)
RM = rm
MOD_KERNEL_PATH = extra
default:
		$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
install:default
		$(MAKE) -C $(KERNELDIR) M=$(PWD) INSTALL_MOD_PATH=$(DESTDIR) INSTALL_MOD_DIR=$(MOD_KERNEL_PATH) modules_install
		$(DEPMOD)

uninstall:
		$(RM) -rf $(DESTDIR)/lib/modules/$(knv)/$(MOD_KERNEL_PATH)
		$(DEPMOD)

clean:
		$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

endif
