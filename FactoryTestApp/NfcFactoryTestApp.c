#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#if 0
#define PRINT_BUF(x,y,z)  {int loop; printf(x); \
                           for(loop=0;loop<z;loop++) printf("%.2x ", y[loop]); \
                           printf("\n");}
#else
#define PRINT_BUF(x,y,z)	
#endif

static char gNfcController_generation = 0;
	
static int openNfc(int * handle)
{
    *handle = open((char const *)"/dev/pn544", O_RDWR);
    if (*handle < 0) return -1;
	return 0;
}

static void closeNfc(int handle)
{
    close(handle);
}

static void reset(int handle)
{
    ioctl(handle, _IOW(0xE9, 0x01, unsigned int), 0);
    usleep(10 * 1000);
    ioctl(handle, _IOW(0xE9, 0x01, unsigned int), 1);
}

static int send(int handle, char *pBuff, int buffLen)
{
	int ret = write(handle, pBuff, buffLen);
	if(ret <= 0) {
		/* retry to handle standby mode */
		ret = write(handle, pBuff, buffLen);
		if(ret <= 0) return 0;
	}
	PRINT_BUF(">> ", pBuff, ret);
	return ret;
}

static int receive(int handle, char *pBuff, int buffLen)
{
    int numRead;
    struct timeval tv;
    fd_set rfds;
    int ret;

    FD_ZERO(&rfds);
    FD_SET(handle, &rfds);
    tv.tv_sec = 2;
    tv.tv_usec = 1;
	ret = select(handle+1, &rfds, NULL, NULL, &tv);
	if(ret <= 0) return 0;

    ret = read(handle, pBuff, 3);
    if (ret <= 0) return 0;
    numRead = 3;
	if(pBuff[2] + 3 > buffLen) return 0;

    ret = read(handle, &pBuff[3], pBuff[2]);
    if (ret <= 0) return 0;
    numRead += ret;

	PRINT_BUF("<< ", pBuff, numRead);

	return numRead;
}

static int transceive(int handle, char *pTx, int TxLen, char *pRx, int RxLen)
{
	if(send(handle, pTx, TxLen) == 0) return 0;
	return receive(handle, pRx, RxLen);
}

static void RfOn (int handle)
{
    char NCIStartDiscovery[] = {0x21, 0x03, 0x09, 0x04, 0x00, 0x01, 0x01, 0x01, 0x02, 0x01, 0x06, 0x01};
    char NCIRfOn[] = {0x2F, 0x3D, 0x02, 0x20, 0x01};
    char Answer[256];
    int NbBytes = 0;
	
	if (gNfcController_generation == 1) {
		printf("Continuous RF ON test - please tap a card\n");
		NbBytes = transceive(handle, NCIStartDiscovery, sizeof(NCIStartDiscovery), Answer, sizeof(Answer));
		if((Answer[0] != 0x41) || (Answer[1] != 0x03) || (Answer[3] != 0x00)) {
			printf("Cannot start discovery loop\n");
			return;
		}
		do {
			NbBytes = receive(handle,  Answer, sizeof(Answer));
		} while ((Answer[0] != 0x61) || ((Answer[1] != 0x05) && (Answer[1] != 0x03)));
	}
	else {
		printf("Continuous RF ON test\n");
		NbBytes = transceive(handle, NCIRfOn, sizeof(NCIRfOn), Answer, sizeof(Answer));
	}
	printf("NFC Controller is now in continuous RF ON mode - Press enter to stop\n");
	fgets(Answer, sizeof(Answer), stdin);
}

static void Prbs (int handle)
{
    char NCIPrbsPN7120[] = {0x2F, 0x30, 0x04, 0x00, 0x00, 0x01, 0x01};
    char NCIPrbsPN7150[] = {0x2F, 0x30, 0x06, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01};
    char Answer[256];
    int NbBytes = 0;
	int tech, bitrate;
	
	printf("PRBS test:\n");
	printf(" Select technology (A=0, B=1, F=2: ");
	scanf("%d", &tech);
	printf(" Select bitrate (106=0, 212=1, 424=2, 848=3: ");
	scanf("%d", &bitrate);
	
	if (gNfcController_generation == 1) {
		NCIPrbsPN7120[3] = tech;
		NCIPrbsPN7120[4] = bitrate;
		NbBytes = transceive(handle, NCIPrbsPN7120, sizeof(NCIPrbsPN7120), Answer, sizeof(Answer));
	}
	else {
		NCIPrbsPN7150[5] = tech;
		NCIPrbsPN7150[6] = bitrate;
		NbBytes = transceive(handle, NCIPrbsPN7150, sizeof(NCIPrbsPN7150), Answer, sizeof(Answer));
	}
	printf("NFC Controller is now in PRBS mode - Press enter to stop\n");
	fgets(Answer, sizeof(Answer), stdin);
}

int main()
{
	int nHandle;
    char NCICoreReset[] = {0x20, 0x00, 0x01, 0x01};
    char NCICoreInit[] = {0x20, 0x01, 0x00};
    char NCIDisableStandby[] = {0x2F, 0x00, 0x01, 0x00};
    char Answer[256];
    int NbBytes = 0;
	
	printf("\n----------------------------\n");
	printf("NFC Factory Test Application\n");
	printf("----------------------------\n");

	if(openNfc(&nHandle) != 0) {
		printf("Cannot connect to PN71xx NFC controller\n");
		return -1;
	}

	reset(nHandle);
	
	NbBytes = transceive(nHandle, NCICoreReset, sizeof(NCICoreReset), Answer, sizeof(Answer));
	NbBytes = transceive(nHandle, NCICoreInit, sizeof(NCICoreInit), Answer, sizeof(Answer));
    if((NbBytes < 19) || (Answer[0] != 0x40) || (Answer[1] != 0x01) || (Answer[3] != 0x00))	{
		printf("Error communicating with PN71xx NFC Controller\n");
		return -1;
	}
    /* Retrieve NXP-NCI NFC Controller generation */
    if (Answer[17+Answer[8]] == 0x08) {
		printf("PN7120 NFC controller detected\n");
		gNfcController_generation = 1;
	}
    else if (Answer[17+Answer[8]] == 0x10) {
		printf("PN7150 NFC controller detected\n");
		gNfcController_generation = 2;
	}
	else {
		printf("Wrong NFC controller detected\n");
		return -1;
	}

	NbBytes = transceive(nHandle, NCIDisableStandby, sizeof(NCIDisableStandby), Answer, sizeof(Answer));
	
	printf("Select the test to run:\n");
	printf("\t 1. Continuous RF ON\n");
	printf("\t 2. PRBS\n");
	printf("Your choice: ");
	scanf("%d", &NbBytes);
	
	switch(NbBytes)	{
		case 1:	RfOn(nHandle);	break;
		case 2: Prbs(nHandle); 	break;
		default: printf("Wrong choice\n");	break;
	}

	fgets(Answer, sizeof(Answer), stdin);
	
	reset(nHandle);
	closeNfc(nHandle);
	
	return 0;
}