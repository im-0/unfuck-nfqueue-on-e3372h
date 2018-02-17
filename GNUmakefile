CROSS_TOOLCHAIN ?= arm-linux-gnu-
BUILD_DIR ?= $(CURDIR)/build-dir
VENDOR_KERNEL_DIR ?= e3372h-vendor-src/vender/modem/system/android/android_4.2_r1/kernel

ROOT_DIR := $(CURDIR)

.PHONY: help
help:
	@echo "Usage:"
	@echo "    make help   # show this help message"
	@echo "    make build  # build module"
	@echo "    make clean  # cleanup"

.PHONY: build
build: $(BUILD_DIR)/.kmod-build-done
	@echo "Result:"
	@ls -lh "./src/unfuck_nfqueue.ko"

.PHONY: clean
clean: clean-kmod
	rm -fr "$(BUILD_DIR)"

$(BUILD_DIR)/.mkdir-done:
	[ -e "$(BUILD_DIR)" ] && \
		rm -fr "$(BUILD_DIR)" || \
		true
	mkdir "$(BUILD_DIR)"
	touch "$(@)"

$(BUILD_DIR)/kernel-src/.copy-done: $(BUILD_DIR)/.mkdir-done
	[ -e "$(BUILD_DIR)/kernel-src" ] && \
		rm -fr "$(BUILD_DIR)/kernel-src" || \
		true
	cp -r "./$(VENDOR_KERNEL_DIR)" "$(BUILD_DIR)/kernel-src"
	cd "$(BUILD_DIR)/kernel-src" && patch -p1 <"$(ROOT_DIR)/kernel.patch"
	cp "$(BUILD_DIR)/kernel-src/include/linux/compiler-gcc4.h" \
		"$(BUILD_DIR)/kernel-src/include/linux/compiler-gcc7.h"
	touch "$(@)"

COMMON_MAKE_OPTS := KERNELRELEASE="3.4.5" \
	ARCH="arm" \
	CROSS_COMPILE="$(CROSS_TOOLCHAIN)" \
	EXTRA_CFLAGS="-I$(ROOT_DIR)/e3372h-vendor-src/vender/platform/hi6921_v711/soc -I$(ROOT_DIR)/e3372h-vendor-src/vender/config/product/hi6921_v711_hilink/config"

$(BUILD_DIR)/kernel-build/.build-done: $(BUILD_DIR)/kernel-src/.copy-done
	[ -e "$(BUILD_DIR)/kernel-build" ] && \
		rm -fr "$(BUILD_DIR)/kernel-build" || \
		true
	mkdir "$(BUILD_DIR)/kernel-build"

	cp "./kernel-config" "$(BUILD_DIR)/kernel-build/.config"

	cd "$(BUILD_DIR)/kernel-src" && make \
		$(COMMON_MAKE_OPTS) \
		O="$(BUILD_DIR)/kernel-build" \
		silentoldconfig prepare headers_install scripts

	touch "$(@)"

$(BUILD_DIR)/.kmod-build-done: $(BUILD_DIR)/kernel-build/.build-done ./src/*.c ./src/Makefile
	cd "./src" && make \
		-C "$(BUILD_DIR)/kernel-build" \
		$(COMMON_MAKE_OPTS) \
		M="$(ROOT_DIR)/src"
	touch "$(@)"

.PHONY:
clean-kmod:
	if [ -e "./src/unfuck_nfqueue.ko" ]; then \
		cd "./src" && make \
			-C "$(BUILD_DIR)/kernel-build" \
			$(COMMON_MAKE_OPTS) \
			M="$(ROOT_DIR)/src" \
			clean; \
	fi
