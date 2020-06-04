MODULE := backends/platform/dcalt

MODULE_OBJS := \
	dcalt.o \
	mutex.o \
	graphics.o \
	access.o \
	saves.o \
	keyboard.o \
	path.o \
	stat.o

# We don't use rules.mk but rather manually update OBJS and MODULE_DIRS.
MODULE_OBJS := $(addprefix $(MODULE)/, $(MODULE_OBJS))
OBJS := $(MODULE_OBJS) $(OBJS)
MODULE_DIRS += $(sort $(dir $(MODULE_OBJS)))
