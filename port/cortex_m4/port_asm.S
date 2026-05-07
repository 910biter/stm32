    .syntax unified
    .thumb

    .extern rtos_current_task
    .extern rtos_schedule_next

    .global port_start_first_task
    .type port_start_first_task, %function
port_start_first_task:
    svc #0
    bx lr

    .global SVC_Handler
    .type SVC_Handler, %function
SVC_Handler:
    ldr r0, =rtos_current_task
    ldr r0, [r0]
    ldr r0, [r0]
    ldmia r0!, {r4-r11}
    msr psp, r0
    movs r0, #2
    msr control, r0
    isb
    ldr lr, =0xFFFFFFFD
    bx lr

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
