    .syntax unified
    .thumb

    .extern rtos_current_task
    .extern rtos_schedule_next

    .global port_start_first_task
    .type port_start_first_task, %function
port_start_first_task:
    ldr r0, =rtos_current_task
    ldr r0, [r0]
    ldr r0, [r0]
    ldmia r0!, {r4-r11}
    add r1, r0, #32
    msr psp, r1
    movs r1, #2
    msr control, r1
    isb
    ldr r1, [r0, #20]
    mov lr, r1
    ldr r1, [r0, #24]
    ldr r0, [r0]
    bx r1

    .global PendSV_Handler
    .type PendSV_Handler, %function
PendSV_Handler:
    mrs r0, psp
    stmdb r0!, {r4-r11}
    ldr r1, =rtos_current_task
    ldr r2, [r1]
    str r0, [r2]

    push {lr}
    bl rtos_schedule_next
    pop {lr}

    ldr r1, =rtos_current_task
    ldr r2, [r1]
    ldr r0, [r2]
    ldmia r0!, {r4-r11}
    msr psp, r0
    bx lr
