global task_switch
task_switch:
    push ebp
    mov ebp, esp ; ebp <-- esp

    push ebx
    push esi
    push edi

    mov eax, esp
    ; 一个任务栈4k大小
    ; 相与得到栈起始地址
    and eax, 0xfffff000 ; current

    mov [eax], esp ; 栈起始位置存下当前esp值

    mov eax, [ebp + 8] ; next 参数，task_t，也是栈的起始地址
    mov esp, [eax] ; 栈顶位置被修改

    pop edi
    pop esi
    pop ebx
    pop ebp
 
    ret ; 返回到esp所指位置
