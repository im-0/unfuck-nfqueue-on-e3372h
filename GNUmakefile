ROOT_DIR := $(CURDIR)

CROSS_TOOLCHAIN ?=
BUILD_DIR ?= $(ROOT_DIR)/build-dir

SOC ?= hi6921_v711
SOC_MODE ?= hilink

VENDOR_PRODUCT_TOPDIR ?= $(ROOT_DIR)/e3372h-vendor-src/vender
VENDOR_PRODUCT_NAME ?= $(SOC)_$(SOC_MODE)
VENDOR_PRODUCT_CONF_DIR ?= $(VENDOR_PRODUCT_TOPDIR)/config/product/$(VENDOR_PRODUCT_NAME)

# Kernel config from vendor sources differs from config from /proc/config.gz
# on device.
#KERNEL_CONFIG ?= $(VENDOR_PRODUCT_CONF_DIR)/os/acore/balongv7r2_defconfig
KERNEL_CONFIG ?= $(ROOT_DIR)/kernel-config

ANDROID_NDK ?= http://dl.google.com/android/ndk/android-ndk-r8b-linux-x86.tar.bz2
ANDROID_NDK_SHA512 ?= a7a4bb00a16a473cec33c185f21b386d72a4d1dc1294f86bfeff95de70c45e0a9b357ac53d482a3b0e8dfc527de9d561e2b121c199c9149662bd4f05e6bef1b2
ANDROID_NDK_DIR ?= android-ndk-r8b
ANDROID_NDK_TOOLCHAIN ?= toolchains/arm-linux-androideabi-4.6/prebuilt/linux-x86/bin/arm-linux-androideabi-

WGET_OPTS ?= -c -t 0 -T 15

VENDOR_COPY_DIR = $(BUILD_DIR)/vendor-src
VENDOR_COPY_KERNEL_DIR = $(VENDOR_COPY_DIR)/modem/system/android/android_4.2_r1/kernel

ANDROID_NDK_FNAME = $(notdir $(ANDROID_NDK))

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

.PHONY: copy-binaries
copy-binary: build
	cp "$(ROOT_DIR)/src/unfuck_nfqueue.ko" "$(ROOT_DIR)/binary/"

$(BUILD_DIR)/.mkdir-done:
	[ -e "$(BUILD_DIR)" ] && \
		rm -fr "$(BUILD_DIR)" || \
		true
	mkdir "$(BUILD_DIR)"
	touch "$(@)"

$(BUILD_DIR)/.download-ndk-done: $(BUILD_DIR)/.mkdir-done
ifeq ($(CROSS_TOOLCHAIN),)
	[ -e "$(BUILD_DIR)/$(ANDROID_NDK_FNAME)" ] && \
		rm -f "$(BUILD_DIR)/$(ANDROID_NDK_FNAME)" || \
		true

	wget $(WGET_OPTS) -O "$(BUILD_DIR)/$(ANDROID_NDK_FNAME)" "$(ANDROID_NDK)"
	printf "%s %s\n" "$(ANDROID_NDK_SHA512)" "$(BUILD_DIR)/$(ANDROID_NDK_FNAME)" | \
		sha512sum -c -
endif

	touch "$(@)"

$(BUILD_DIR)/.unpack-ndk-done: $(BUILD_DIR)/.download-ndk-done
ifeq ($(CROSS_TOOLCHAIN),)
	[ -e "$(BUILD_DIR)/$(ANDROID_NDK_DIR)" ] && \
		rm -fr "$(BUILD_DIR)/$(ANDROID_NDK_DIR)" || \
		true

	tar -C "$(BUILD_DIR)" -xf "$(BUILD_DIR)/$(ANDROID_NDK_FNAME)"

endif

	touch "$(@)"

$(VENDOR_COPY_DIR)/.copy-done:: $(BUILD_DIR)/.mkdir-done $(ROOT_DIR)/patches/*.patch
	[ -e "$(VENDOR_COPY_DIR)/kernel-src" ] && \
		rm -fr "$(VENDOR_COPY_DIR)/kernel-src" || \
		true
	cp -r "$(VENDOR_PRODUCT_TOPDIR)" "$(VENDOR_COPY_DIR)"

	cd "$(VENDOR_COPY_DIR)"; \
	ls -1 $(ROOT_DIR)/patches/*.patch | sort -n | while read patch_file; do \
		printf "Applying patch: %s\n" $$( basename "$${patch_file}" ); \
		patch -p2 <"$${patch_file}"; \
	done

	for gcc_ver in 5 6 7; do \
		cp -v "$(VENDOR_COPY_KERNEL_DIR)/include/linux/compiler-gcc4.h" \
			"$(VENDOR_COPY_KERNEL_DIR)/include/linux/compiler-gcc$${gcc_ver}.h"; \
	done

	touch "$(@)"

ifeq ($(CROSS_TOOLCHAIN),)
CROSS_TOOLCHAIN = $(BUILD_DIR)/$(ANDROID_NDK_DIR)/$(ANDROID_NDK_TOOLCHAIN)
endif

COMMON_MAKE_OPTS := KERNELRELEASE="3.4.5" \
	ARCH="arm" \
	CROSS_COMPILE="$(CROSS_TOOLCHAIN)" \
	EXTRA_CFLAGS="-fno-pic" \
    BALONG_TOPDIR=$(VENDOR_COPY_DIR) \
	OBB_PRODUCT_NAME=$(VENDOR_PRODUCT_NAME) \
	CFG_PLATFORM=$(SOC)

$(BUILD_DIR)/kernel-build/.build-done: $(BUILD_DIR)/.unpack-ndk-done $(VENDOR_COPY_DIR)/.copy-done $(KERNEL_CONFIG)
	[ -e "$(BUILD_DIR)/kernel-build" ] && \
		rm -fr "$(BUILD_DIR)/kernel-build" || \
		true
	mkdir "$(BUILD_DIR)/kernel-build"

	cp "$(KERNEL_CONFIG)" "$(BUILD_DIR)/kernel-build/.config"

	cd "$(VENDOR_COPY_KERNEL_DIR)" && yes "n" | $(MAKE) -j1 \
		$(COMMON_MAKE_OPTS) \
		O="$(BUILD_DIR)/kernel-build" \
		oldconfig

	cd "$(VENDOR_COPY_KERNEL_DIR)" && $(MAKE) \
		$(COMMON_MAKE_OPTS) \
		O="$(BUILD_DIR)/kernel-build" \
		prepare headers_install scripts

	touch "$(@)"

$(BUILD_DIR)/.kmod-build-done: $(BUILD_DIR)/kernel-build/.build-done ./src/*.c ./src/Makefile
	touch $(BUILD_DIR)/.kmod-build-started

	cd "./src" && $(MAKE) \
		-C "$(BUILD_DIR)/kernel-build" \
		$(COMMON_MAKE_OPTS) \
		M="$(ROOT_DIR)/src"

	$(CROSS_TOOLCHAIN)strip --strip-debug "./src/unfuck_nfqueue.ko"

	touch "$(@)"

.PHONY:
clean-kmod:
	if [ -e "$(BUILD_DIR)/.kmod-build-started" ]; then \
		cd "./src" && $(MAKE) \
			-C "$(BUILD_DIR)/kernel-build" \
			$(COMMON_MAKE_OPTS) \
			M="$(ROOT_DIR)/src" \
			clean; \
	fi

	rm -f "$(BUILD_DIR)/.kmod-build-started"
