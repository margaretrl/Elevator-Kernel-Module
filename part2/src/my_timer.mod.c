#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

#ifdef CONFIG_UNWINDER_ORC
#include <asm/orc_header.h>
ORC_HEADER;
#endif

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x9ec6ca96, "ktime_get_real_ts64" },
	{ 0x365acda7, "set_normalized_timespec64" },
	{ 0x656e4a6e, "snprintf" },
	{ 0xa916b694, "strnlen" },
	{ 0xcbd4898c, "fortify_panic" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0x6b10bee1, "_copy_to_user" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x466b010, "proc_remove" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x623ae57d, "proc_create" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0xb93f0c72, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "55CEB43E0FE0E870FC11852");
