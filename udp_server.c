#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <sys/time.h>
#include <arpa/inet.h>

enum CMD_UPLOAD
{
    CMD_NETWORK_RST =  0xC0,
    CMD_OPEN_DOOR   =  0xE0,
    CMD_CLOSE_DOOR  =  0xE1,
    CMD_ADD_KEY     =  0xE2,
    CMD_DEL_KEY     =  0xE3,
    CMD_DEL_ALL     =  0xE4,
    CMD_ALARM       =  0xE5,
    CMD_DEFAULT     =  0xEF,
};

enum
{
    KEY_FIN = 0x00,
    KEY_PWD,
    KEY_CARD,
    KEY_MECH,
    KEY_FIRE,
    KEY_REMOTE,
};

#define ALARM_NORMAL    0
#define ALARM_UNLOCK    1
#define ALARM_DESTROY   2
#define ALARM_PWDFAIL   3
#define ALARM_FINFAIL   4
#define ALARM_CARDFAIL  5
#define ALARM_RAIDWARN  6
#define ALARM_FIREWARN  7
#define ALARM_IRWARN    8

#define BUFSIZE (1024)
#define SERV_PORT (8300)

char* get_locktype_str( int type )
{
    if(type == KEY_FIN )
    {
        return "Finger";
    }
    else if( type == KEY_PWD )
    {
        return "Password";
    }
    else if( type == KEY_CARD )
    {
        return ("Card");
    }
    else if( type == KEY_MECH )
    {
        return "Mech";
    }
    else if( type == KEY_FIRE )
    {
        return "Fire";
    }
    else if( type == KEY_REMOTE )
    {
        return "NB_Remote";
    }
    else
    {
        return "Unknown";
    }
}

char* get_alarm_str( int type )
{
    switch( type )
    {
        case ALARM_UNLOCK:
            return "Unlock";
        case ALARM_DESTROY:
            return "Destroy";
        case ALARM_PWDFAIL:
            return "PWDFail";           
        case ALARM_FINFAIL:
            return "FingerFail";
        case ALARM_CARDFAIL:
            return "CardFail";
        case ALARM_RAIDWARN:
            return "RaidWarn";
        case ALARM_FIREWARN:
            return "FireWarn";
        case ALARM_IRWARN:
            return "IRWarn";
        default:
            return "";
    }
}


void parse_nb_data(unsigned char* data, int data_len)
{
    int nb_data_len = 0;
    int i;
    int index = 0;
    if(data == NULL)
        return;

    if(data_len == 4 && data[0] == '\r' && data[1] == '\n'
        &&  data[2] == '\r' && data[3] == '\n')
    {
        printf("NB Module Wake up!\n");
        return;
    }
            

    if( data[index++] != 0xAA )
    {
        printf("Not NB_Upload info\n");
        return;
    }

    nb_data_len = data[index++];

    printf("Device Imei:");
    for( i = 0; i < 15; i++ )
        printf("%c",data[index++]);
    printf("\n");

    switch( data[index] )
    {
        index++;
        case CMD_OPEN_DOOR:
            printf("%s open\n", get_locktype_str(data[++index]));
            if(data[++index] != '\0')
                printf("LockNum: %d\n", data[index]);
            break;

        case CMD_CLOSE_DOOR:
            printf("Door close\n");
            break;

        case CMD_ADD_KEY:
            printf("Add %s key\n",get_locktype_str(data[++index]));
            if(data[++index] != '\0')
                printf("Add KeyNum: %d\n", data[index]);
            break;

        case CMD_DEL_KEY:
            printf("Delete %s key\n",get_locktype_str(data[++index]));
            if(data[++index] != '\0')
                printf("Del KeyNum: %d\n", data[index]);
            break;

        case CMD_DEL_ALL:
            printf("Device restore\n");
            break;

        case CMD_ALARM:
            printf("%s Alarm!!!\n",get_alarm_str(data[++index]));
            break;

        default:
            printf("unkonwn cmd\n");
            break;

    }

}


int main(int argc,char* argv[])
{
    int nSocketFd = 0;
    char szBuff[BUFSIZ] = {0};
    int nRdSocketLen = 0;
    int i = 0;

    fd_set rfds;
    struct timeval tv;
    int maxfd = 0;
    int retval = 0;
    int nWrSocketLen = 0;
    time_t t;


    struct sockaddr_in servaddr, cliaddr;
    socklen_t cliaddr_len;


    /** 获得socket的文件描述符 */
    nSocketFd = socket(AF_INET,SOCK_DGRAM,0);
    if(-1 == nSocketFd)
    {
        perror("create socket failed : ");
        return -1;
    }


    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);

    if( 0 > bind(nSocketFd, (struct sockaddr*)&servaddr, sizeof(servaddr)))
    {
        perror("bind failed: ");
        return -1;
    }
    printf("Accepting connections ...\n\n"); 

    if(nSocketFd > maxfd)
    {
        maxfd = nSocketFd;
    }

    while(1)
    {
        FD_ZERO(&rfds);
        FD_SET(0,&rfds);//设置键盘响应
        FD_SET(nSocketFd,&rfds);
        tv.tv_sec = 10;
        tv.tv_usec = 0;
        retval = select(maxfd+1,&rfds,NULL,NULL,&tv);
        if(retval == -1)
        {
            perror("select:");
            return 0;
        }

        if(retval == 0) //没有响应
        {
            continue;
        }

        memset(szBuff,0,BUFSIZ);
        if(FD_ISSET(0,&rfds)) //是键盘响应
        {
            /** 从终端读取数据 */
            read(0,szBuff,BUFSIZ);

            /** 把从终端读取的数据发送出去 */
            nWrSocketLen = sendto(nSocketFd, szBuff, strlen(szBuff), 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
            if(nWrSocketLen > 0)
            {
                printf("send message successful\n\n");
            }
        }
        if(FD_ISSET(nSocketFd,&rfds))
        {
            /** 把服务器端的数据发送过来 */
            time(&t);
            cliaddr_len = sizeof(cliaddr);  
            nRdSocketLen = recvfrom(nSocketFd, szBuff, BUFSIZE, 0, (struct sockaddr*)&cliaddr, &cliaddr_len); 
            //nRdSocketLen = recvfrom(nSocketFd,szBuff,BUFSIZ);
            if(nRdSocketLen > 0)
            {
                printf("Time:%s", ctime(&t));
                printf("Recv data from: [%s:%d]\n",inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port) );
                printf("Raw Data: ");
                for( i = 0; i < nRdSocketLen; i++)
                    printf("%02x",(unsigned char)szBuff[i] );
                printf("\n");
                //printf("read data:%s\n",szBuff);
                parse_nb_data((unsigned char*)szBuff,nRdSocketLen);
                printf("\n");
            }
            else
                printf("recvfrom failed\n");
        }


    }

    return 0;
}
