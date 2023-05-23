/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
// baudrate mudado para 9600
#define BAUDRATE B9600
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

typedef enum {
    START,
    FLAG_RCV,
    A_RCV,
    C_RCV,
    BCC_OK,
    ENVIA_UA,
    UA_ENVIADO,
} CONTROLO;

CONTROLO CONT = START;

volatile int STOP = FALSE;

int verifica(char readBuf[]) {
    if(readBuf[2]==0x03) {
        return 1;
    }
    else
        return 0;
}

int main(int argc, char **argv)
{
    int fd, c, res, res_controlo;
    struct termios oldtio, newtio;
    char buf[255], readBuf[5], sendUA_buf[5];

    // mudei para ttyS10 e ttyS11 para experimentar em casa
    if ((argc < 2) ||
        ((strcmp("/dev/ttyS10", argv[1]) != 0) &&
         (strcmp("/dev/ttyS11", argv[1]) != 0)))
    {
        printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
        exit(1);
    }

    /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
    */

    fd = open(argv[1], O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(argv[1]);
        exit(-1);
    }

    if (tcgetattr(fd, &oldtio) == -1)
    { /* save current port settings */
        perror("tcgetattr");
        exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = 0; /* inter-character timer unused */
    newtio.c_cc[VMIN] = 5;  /* blocking read until 5 chars received */

    /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) próximo(s) caracter(es)
    */

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }
    

    /*
    O ciclo WHILE deve ser alterado de modo a respeitar o indicado no guião
    */

    //Receber o set
    while(CONT != UA_ENVIADO)
    {
        
        switch(CONT) 
        {
            case START :
                res_controlo = read(fd,readBuf,5);
                if (readBuf[0] == 0x5c)
                {
                    printf("FLAG_SET recebida\n");
					CONT = FLAG_RCV;
                }
                else
                {
                    CONT = START;
                }
            break;

            case FLAG_RCV:
            if (readBuf[1] == 0x01)
                {
                    printf("Address_SET recebido\n");
					CONT = A_RCV;
                }
                else
                {
                    CONT = START;
                }    

            break;

            case A_RCV:
                //res_controlo = read(fd,readBuf,5);    
				
				if (readBuf[2] == 0x03)
                {
                    printf("Control_SET recebido\n");
					CONT = C_RCV;
                }
                else
                {
                    CONT = START;
                }
            break;

            case C_RCV:
                if (readBuf[3] == (readBuf[1]^readBuf[2]))
                {
                    printf("BCC1_SET correto\n");
					CONT = BCC_OK;
                }
                else
                {
                    CONT = START;
                }
            break;

            case BCC_OK:
                if (readBuf[4] == 0x5c)
                {
                    printf("FLAG_SET recebida\n");
					CONT = ENVIA_UA;
                }
                else
                {
                    CONT = START;
                }
            break;


            case ENVIA_UA:
                //Envio do UA
                printf("Chegou a envia_ua\n");
                sendUA_buf[0] = 0x5c;
                sendUA_buf[1] = 0x01;
                sendUA_buf[2] = 0x07;
                sendUA_buf[3] = sendUA_buf[1]^sendUA_buf[2];
                sendUA_buf[4] = 0x5c;

                res = write(fd,sendUA_buf,5);
                printf("UA enviado\n");
                CONT = UA_ENVIADO;
            break;

            case UA_ENVIADO:

            break;
                 
        }
    }



    tcsetattr(fd, TCSANOW, &oldtio);
    close(fd);
    return 0;
}