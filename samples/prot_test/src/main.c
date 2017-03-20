#include <zephyr.h>
#include <string.h>
#include <stdlib.h>
#include <misc/printk.h>
#include <shell/shell.h>

#include "targets.h"

/* prot test command help texts */
#define WRITE_RO_CMD_HELP  "Write to read-only data."
#define WRITE_TEXT_CMD_HELP  "Write to text."
#define EXEC_DATA_CMD_HELP  "Execute from data."
#define EXEC_STACK_CMD_HELP  "Execute from stack."
#if (CONFIG_HEAP_MEM_POOL_SIZE > 0)
#define EXEC_HEAP_CMD_HELP  "Execute from heap."
#endif
#ifdef CONFIG_ARM_CORE_MPU
#define DISABLE_MPU_CMD_HELP  "Disable MPU."
#define ENABLE_MPU_CMD_HELP  "Enable MPU."
#endif

#ifdef CONFIG_CPU_CORTEX_M
#include <arch/arm/cortex_m/cmsis.h>
/* Must clear LSB of function address to access as data. */
#define FUNC_TO_PTR(x) (void *)((uintptr_t)(x) & ~0x1)
/* Must set LSB of function address to call in Thumb mode. */
#define PTR_TO_FUNC(x) (int (*)(int))((uintptr_t)(x) | 0x1)
/* Flush preceding data writes and instruction fetches. */
#define DO_BARRIERS() do { __DSB(); __ISB(); } while (0)
#else
#define FUNC_TO_PTR(x) (void *)(x)
#define PTR_TO_FUNC(x) (int (*)(int))(x)
#define DO_BARRIERS() do { } while (0)
#endif

static int __attribute__((noinline)) add_one(int i)
{
	return (i + 1);
}

static void execute_from_buffer(u8_t *dst)
{
	void *src = FUNC_TO_PTR(add_one);
	int (*func)(int i) = PTR_TO_FUNC(dst);
	int i = 1;

	/* Copy add_one() code to destination buffer. */
	memcpy(dst, src, BUF_SIZE);
	DO_BARRIERS();

	/*
	 * Try executing from buffer we just filled.
	 * Optimally, this triggers a fault.
	 * If not, we check to see if the function
	 * returned the expected result as confirmation
	 * that we truly executed the code we wrote.
	 */
	printk("trying to call code written to %p\n", func);
	i = func(i);
	printk("returned from code at %p\n", func);
	if (i == 2) {
		printk("FAIL: Execute from target buffer succeeded!\n");
	} else {
		printk("UNEXPECTED: Did not fault but also did not get expected return value!\n");
	}
}

static int shell_cmd_write_ro(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	u32_t *ptr = (u32_t *)&rodata_var;

	/*
	 * Try writing to rodata.  Optimally, this triggers a fault.
	 * If not, we check to see if the rodata value actually changed.
	 */
	printk("trying to write to rodata at %p\n", ptr);
	*ptr = ~RODATA_VALUE;

	DO_BARRIERS();

	if (*ptr == RODATA_VALUE) {
		printk("rodata value still the same\n");
	} else if (*ptr == ~RODATA_VALUE) {
		printk("FAIL: rodata modified!\n");
	} else {
		printk("FAIL: something went wrong!\n");
	}

	return 0;
}

static int shell_cmd_write_text(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	void *src = FUNC_TO_PTR(add_one);
	void *dst = FUNC_TO_PTR(overwrite_target);
	int i = 1;

	/*
	 * Try writing to a function in the text section.
	 * Optimally, this triggers a fault.
	 * If not, we try calling the function after overwriting
	 * to see if it returns the expected result as
	 * confirmation that we truly executed the code we wrote.
	 */
	printk("trying to write to text at %p\n", dst);
	memcpy(dst, src, BUF_SIZE);
	DO_BARRIERS();
	i = overwrite_target(i);
	if (i == 2) {
		printk("FAIL: Overwrite of text succeeded!\n");
	} else {
		printk("UNEXPECTED: Did not fault but also did not get expected return value!\n");
	}

	return 0;
}

static int shell_cmd_exec_data(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	execute_from_buffer(data_buf);
	return 0;
}

static int shell_cmd_exec_stack(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	u8_t stack_buf[BUF_SIZE] __aligned(sizeof(int));

	execute_from_buffer(stack_buf);
	return 0;
}

#if (CONFIG_HEAP_MEM_POOL_SIZE > 0)
static int shell_cmd_exec_heap(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	u8_t *heap_buf = k_malloc(BUF_SIZE);

	execute_from_buffer(heap_buf);
	k_free(heap_buf);
	return 0;
}
#endif

#ifdef CONFIG_ARM_CORE_MPU

#include <arch/arm/cortex_m/mpu/arm_core_mpu.h>

static int shell_cmd_disable_mpu(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	arm_core_mpu_disable();
	return 0;
}

static int shell_cmd_enable_mpu(int argc, char *argv[])
{
	arm_core_mpu_enable();
	return 0;
}

#endif

#define SHELL_MODULE "prot_test"

static struct shell_cmd commands[] = {
	{ "write_ro", shell_cmd_write_ro, WRITE_RO_CMD_HELP},
	{ "write_text", shell_cmd_write_text, WRITE_TEXT_CMD_HELP },
	{ "exec_data", shell_cmd_exec_data, EXEC_DATA_CMD_HELP },
	{ "exec_stack", shell_cmd_exec_stack, EXEC_STACK_CMD_HELP },
#if (CONFIG_HEAP_MEM_POOL_SIZE > 0)
	{ "exec_heap", shell_cmd_exec_heap, EXEC_HEAP_CMD_HELP },
#endif
#ifdef CONFIG_ARM_CORE_MPU
	{ "disable_mpu", shell_cmd_disable_mpu, DISABLE_MPU_CMD_HELP },
	{ "enable_mpu", shell_cmd_enable_mpu, ENABLE_MPU_CMD_HELP },
#endif
	{ NULL, NULL, NULL }
};

void main(void)
{
	SHELL_REGISTER(SHELL_MODULE, commands);
	shell_register_default_module(SHELL_MODULE);
}
