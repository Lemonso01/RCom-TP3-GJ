/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
//mudamos o baudrate para 9600
#define BAUDRATE B9600
#define MODEMDEVICE "/dev/ttyS11"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;

int main(int argc, char** argv)
{
    int fd,c, res, res2;
    struct termios oldtio,newtio;
    char buf[255];
    char buf2[255];
    int i, sum = 0, speed = 0;
    // mudei para ttyS10 e ttyS11 para experimentar socat em casa
    if ( (argc < 2) ||
         ((strcmp("/dev/ttyS10", argv[1])!=0) &&
          (strcmp("/dev/ttyS11", argv[1])!=0) )) {
        printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS11\n");
        exit(1);
    }


    /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
    */


    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd < 0) { perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
        perror("tcgetattr");
        exit(-1);
    }

    bzero(&newtio, sizeof(newtio)); // coloca o tamanho do newtio 
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD; // flag de controlo
    newtio.c_iflag = IGNPAR;  // flag de configuracao da recepcao 
    newtio.c_oflag = 0; // flag de configuracao da transmissao 

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 5;   /* blocking read until 5 chars received, corresponde ao numero de caracteres
                                de controlo mudado para ler de 1 a 1*/



    /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) próximo(s) caracter(es)
    */


    tcflush(fd, TCIOFLUSH); //flushes both input and output data

    if (tcsetattr(fd,TCSANOW,&newtio) == -1) { // 
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");


    //colocar 255 caracteres
    for (i = 0; i < 255; i++) {
        
        buf[i] = getchar();
        if(buf[i] == '!')
            break;
    }

    /*testing*/
    //buf[10] = '!';

    res = write(fd,buf,i);
    printf("%d bytes written\n", res);

    printf("Termios structure retrieved\n");

    while (STOP==FALSE) {       /* loop for input */
        res2 = read(fd,buf2,255);   /* returns after 1 char have been input */
        buf2[res2]=0;               /* so we can printf... */
        printf(":%s:%d\n", buf2, res2); 

        if (buf[i] == '!') STOP=TRUE;
    }


    /*
    O ciclo FOR e as instruções seguintes devem ser alterados de modo a respeitar
    o indicado no guião
    */
   // ./writenoncanonical "/dev/ttyS0"


    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }


    close(fd);
    return 0;
}
