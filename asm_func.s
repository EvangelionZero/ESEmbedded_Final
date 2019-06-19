.syntax unified

.global	read_ctrl
read_ctrl:
	mrs	r0,	control
	bx	lr

.global	start_user
start_user:
	movs	lr,	r0//r0存的是task0的entry
	msr	psp,	r1//將r1(本task的空間最上層位置）設為psp

	movs	r3,	#0b11
	msr	control,	r3//之前在svc裡設置過的東東
	isb

	bx	lr

.type systick_handler, %function
.global systick_handler
//exception trigger the follow Hardware Mechanism 
//Hardware Mechanism: creat stack frame & EXC_RETURN to LR & handler entry to PC
systick_handler:
	
	//save lr (EXC_RETURN) to main stack
	push {lr}

	//save r4-r11 to user stack
	mrs	r0,	psp             //r0此時為stack frame+R11~R4的最底下位置,也就是init_psp-16
	stmdb	r0!,{r4-r11}	//此指令的r0代表位置
							//重複（存取數字後r0加一個資料長度單位（上面一個位置））
							//從數字最小的暫存器開始存上去
							//最後r0會停在數字最高的暫存器存放地址的上面一個位置

	//pass psp of curr task by r0 and get psp of the next task
	bl	sw_task		//跳到切換task的函式<--回到main.c檔看,很重要
	//psp of the next task is now in r0

	//restore r4~r11 from stack of the next task
	ldmia r0!,{r4-r11}		//此指令的r0代表位置
							//重複（r0減一個資料長度單位（下面一個位置）後載入數字）
							//從數字最大的暫存器開始載入下去
							//最後r0會停在數字最低的暫存器存放地址位置

	//modify psp
	msr psp,r0 //after sw_task function,psp of the next task is now in r0

	//restore lr (EXC_RETURN)
	pop {lr}

	bx	lr//"EXC_RETURN" trigger the follow Hardware Mechanism 

//Hardware Mechanism: restore stack frame & return address to PC
