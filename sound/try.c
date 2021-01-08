#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

char MPLAYER_CTRL[] = "/tmp/mplayer-control";

int startMPlayerInBackground()
{
    pid_t processId = fork();

    if (processId == 0)
    {
        printf("running mplayer\n");
        char cmd[256];
        snprintf(cmd, 256, "mplayer -quiet -fs -slave -idle -input file=%s", MPLAYER_CTRL);
        int status = system(cmd);
        printf("mplayer ended with status %d\n", status);
        exit(status);
    }
    else 
    {
        return processId;
    }
}

void send(char* cmd)
{
    int fdes = open(MPLAYER_CTRL, O_WRONLY);
    write(fdes, cmd, strlen(cmd));
    close(fdes);
}

int main(int argc, char *args[])
{
    int press = 0; 
    unlink(MPLAYER_CTRL);
    int res = mknod(MPLAYER_CTRL, S_IFIFO|0777, 0);

    pid_t processId = startMPlayerInBackground();

    if (processId < 0) 
    {
        printf("failed to start child process\n");
    }
    else
    {
	while(scanf("%d", &press)) 
        send("loadfile sound.mp3\n");
        //sleep(2);
        //send("pause\n");
        //sleep(1);
        //send("pause\n");
        //sleep(2);
        //send("quit\n");
    }
    return 0;
}
