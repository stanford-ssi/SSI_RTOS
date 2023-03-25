.syntax unified
.cpu cortex-m4
.fpu softvfp

.thumb

.global rpi_init_trampoline
.type rpi_init_trampoline, %function
rpi_init_trampoline:
    mov r0, r5
    blx r4
.size rpi_init_trampoline, .-rpi_init_trampoline
