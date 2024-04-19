	.file	"stack_overflow.c"
	.text
	.section	.rodata
.LC0:
	.string	"success\n"
	.text
	.globl	function1
	.type	function1, @function
function1:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	$.LC0
	call	printf
	addl	$4, %esp
	nop
	leave
	ret
	.size	function1, .-function1
	.section	.rodata
.LC1:
	.string	"%d\n"
.LC2:
	.string	"%x %x\n"
	.text
	.globl	t
	.type	t, @function
t:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$12, %esp
	movl	$t-15, %eax
	movb	%al, 3(%ebp)
	pushl	8(%ebp)
	pushl	$.LC1
	call	printf
	addl	$8, %esp
	pushl	$function1
	pushl	$function1
	pushl	$.LC2
	call	printf
	addl	$12, %esp
	nop
	leave
	ret
	.size	t, .-t
	.globl	main
	.type	main, @function
main:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$8, %esp
	movl	$1, -4(%ebp)
	movl	$2, -8(%ebp)
	pushl	-8(%ebp)
	pushl	-4(%ebp)
	call	t
	addl	$8, %esp
	movl	$0, %eax
	leave
	ret
	.size	main, .-main
	.section	.note.GNU-stack,"",@progbits
