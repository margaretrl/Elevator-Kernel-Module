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
	{ 0x4dfa8d4b, "mutex_lock" },
	{ 0x22137741, "kthread_create_on_node" },
	{ 0xca8a6949, "wake_up_process" },
	{ 0x122c3a7e, "_printk" },
	{ 0x3213f038, "mutex_unlock" },
	{ 0xf9a482f9, "msleep" },
	{ 0xb3f7646e, "kthread_should_stop" },
	{ 0xd9db8fff, "STUB_start_elevator" },
	{ 0xa08a3aa8, "STUB_issue_request" },
	{ 0x88d8d38e, "STUB_stop_elevator" },
	{ 0x623ae57d, "proc_create" },
	{ 0x9f37c7dd, "remove_proc_entry" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0x619cb7dd, "simple_read_from_buffer" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x2f110687, "kthread_stop" },
	{ 0x466b010, "proc_remove" },
	{ 0xab4eceaf, "kmalloc_caches" },
	{ 0x1dfd3b19, "kmalloc_trace" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0xb93f0c72, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "D012287172A14CB76660A1D");
