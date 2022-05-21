#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

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
	{ 0x9f593a9b, "module_layout" },
	{ 0x36d04f2b, "cdev_del" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0xb50c5cd2, "class_destroy" },
	{ 0x25ba506b, "device_destroy" },
	{ 0x56b94690, "cdev_add" },
	{ 0xa9c6a98b, "cdev_init" },
	{ 0xb088826d, "cdev_alloc" },
	{ 0x865e309f, "device_create" },
	{ 0x827ba31, "__class_create" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0xc959d152, "__stack_chk_fail" },
	{ 0x13c49cc2, "_copy_from_user" },
	{ 0x6b10bee1, "_copy_to_user" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0x69acdf38, "memcpy" },
	{ 0x37a0cba, "kfree" },
	{ 0xed5f6e, "kmem_cache_alloc_trace" },
	{ 0x25931311, "kmalloc_caches" },
	{ 0xeb233a45, "__kmalloc" },
	{ 0x56470118, "__warn_printk" },
	{ 0xc5850110, "printk" },
	{ 0xbdfb6dbb, "__fentry__" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "067832BD62D97373FEF8FFF");
