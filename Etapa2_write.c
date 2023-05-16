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

typedef enum{
		BASE,
        ENVIA_SET,
        ESPERA_UA,
		RECEBE_UA,
	} CONTROLO;


//Estados iniciais
CONTROLO CONT = BASE;

//Funções auxiliares
int verifica (char buffer[5])
{
    if(buffer[2] == 0x07)
        return 1;
    else
        return 0;
}

int main(int argc, char** argv)
{
    int fd,c, res, res2, res_controlo;
    struct termios oldtio,newtio;
    char buf[255];
    char buf2[255];
    char buf_set[5], buf_controlo[5];
    int i, sum = 0, speed = 0, RECEBE = 0;
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

    

    //Leitura da resposta

    while (CONT != RECEBE_UA) {        //loop for input 

		switch (CONT) {
			case BASE :
					
				CONT = ENVIA_SET;
			break;
            
            case ENVIA_SET:
            //Envio do SET


                buf_set[0] = 0x5c;
                buf_set[1] = 0x01;
                buf_set[2] = 0x03;
                buf_set[3] = buf_set[1]^buf_set[2];
                buf_set[4] = 0x5c;

                res = write(fd,buf_set,5);
                printf("SET enviado\n");
                CONT = ESPERA_UA;
            break;
                
			
			case ESPERA_UA :

                res_controlo = read(fd,buf_controlo,5);    
                printf("Leu buffer\n");
				
				if (buf_controlo[2] == 0x07)
                {
                    printf("UA recebido\n");
					CONT = RECEBE_UA;
                }

			break;

            case RECEBE_UA:
                
            break;
		} //end case

    }


    





    /*
    O ciclo FOR e as instruções seguintes devem ser alterados de modo a respeitar
    o indicado no guião
    */
   // ./writenoncanonical "/dev/ttyS0" */


    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }


    close(fd);
    return 0;
}
