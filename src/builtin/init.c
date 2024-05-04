#include <yui/types.h>
#include <yui/stdio.h>
#include <yui/syscall.h>

int main()
{
    if (getpid() != 1)
    {
        printf("init already running...\n");
        return 0;
    }

    printf("init start running...\n");

    while (true)
    {
        u32 status;
        pid_t pid = fork();
        if (pid)
        {
            pid_t child = waitpid(pid, &status);
            printf("wait pid %d status %d %d\n", child, status, time());
        }
        else
        {
            int err = execve("/bin/osh.out", NULL, NULL);
            printf("execve /bin/osh.out error %d\n", err);
            exit(err);
        }
    }
    return 0;
}