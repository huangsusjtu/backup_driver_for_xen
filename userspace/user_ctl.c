#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include<sys/ioctl.h>
#include<unistd.h>

//cmd from user space
#define RECODR   15
#define STOP     16
#define ROLLBACK 17
//

int main( void )
{
int fp = 0 ;
int res;
fp = open( "/dev/xenbackup", O_RDWR, S_IRUSR|S_IWUSR );
if ( fp<=0 )
{
	printf("Open device failed\n");
	return -1;
}

res=ioctl(fp, 16, 0);
if( res )
{
	printf("Err ioctl, %d\n",res);
}
printf("Good ioctl\n");

close(fp);
}
