
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "../../include/qisr.h"
#include "../../include/msp_cmn.h"
#include "../../include/msp_errors.h"


void run_iat(const char* src_wav_filename ,const char* des_text_filename ,  const char* param)
{
	char rec_result[1024*4] = {0};
	const char *sessionID;
	FILE *f_pcm = NULL;
	FILE* fout = NULL;
	char *pPCM = NULL;
	int lastAudio = 0 ;
	int audStat = 2 ;
	int epStatus = 0;
	int recStatus = 0 ;
	long pcmCount = 0;
	long pcmSize = 0;
	int ret=0 ;

	sessionID = QISRSessionBegin(NULL, param, &ret);

	fout = fopen( des_text_filename , "ab");
	f_pcm = fopen(src_wav_filename, "rb");
	if (NULL != f_pcm) {
		fseek(f_pcm, 0, SEEK_END);
		pcmSize = ftell(f_pcm);
		fseek(f_pcm, 0, SEEK_SET);
		pPCM = (char *)malloc(pcmSize);
		fread((void *)pPCM, pcmSize, 1, f_pcm);
		fclose(f_pcm);
		f_pcm = NULL;
	}
	while (1) {
		unsigned int len = 6400;
		if (pcmSize < 12800) {
			len = pcmSize;
			lastAudio = 1;
		}
		audStat = 2;
		if (pcmCount == 0)
			audStat = 1;
		if (0) {
			if (audStat == 1)
				audStat = 5;
			else
				audStat = 4;
		}
		if (len<=0)
		{
			break;
		}
		printf("csid=%s,count=%d,aus=%d,",sessionID,pcmCount/len,audStat);
		ret = QISRAudioWrite(sessionID, (const void *)&pPCM[pcmCount], len, audStat, &epStatus, &recStatus);
		printf("eps=%d,rss=%d,ret=%d\n",epStatus,recStatus,ret);
		if (ret != 0)
			break;
		pcmCount += (long)len;
		pcmSize -= (long)len;
		if (recStatus == 0) {
			const char *rslt = QISRGetResult(sessionID, &recStatus, 0, &ret);
            if (ret !=0)
			{
				printf("QISRGetResult Failed,ret=%d\n",ret);
				break;
			}
			if (NULL != rslt)
				printf("%s\n", rslt);
		}
		if (epStatus == MSP_EP_AFTER_SPEECH)
			break;
		usleep(150000);
	}
	ret=QISRAudioWrite(sessionID, (const void *)NULL, 0, 4, &epStatus, &recStatus);
	if (ret !=0)
	{
		printf("QISRAudioWrite Failed,ret=%d\n",ret);
	}
	free(pPCM);
	pPCM = NULL;
	while (recStatus != 5 && ret == 0) {
		const char *rslt = QISRGetResult(sessionID, &recStatus, 0, &ret);
		if (NULL != rslt)
		{
			strcat(rec_result,rslt);
			fwrite(rslt, 1, strlen(rslt), fout);
			
		}
		usleep(150000);
	}
	fclose(fout);
    ret=QISRSessionEnd(sessionID, NULL);
	if(ret !=MSP_SUCCESS)
	{
		printf("QISRSessionEnd Failed, ret=%d\n",ret);
	}

	printf("=============================================================\n");
	printf("The result is: %s\n",rec_result);
	printf("=============================================================\n");
	usleep(100000);
}

int main(int argc, char* argv[])
{
	///APPID请勿随意改动
	const char* login_config = "appid = 53be54f9,work_dir =   .  ";
	const char* param1 = "sub=iat,ssm=1,auf=audio/L16;rate=16000,aue=speex,ent=sms16k,nbest=5";//最多5个候选项，格式只能为json，编码只能为utf8
	const char* param2 = "sub=iat,ssm=1,auf=audio/L16;rate=16000,aue=speex,ent=sms16k,nbest=1";//最多2个候选项，格式只能为json，编码只能为utf8,nbest的取值范围为1~5
	const char* output_file = "iat_result.txt";
	int ret = 0;
	char key = 0;

	//用户登录
	ret = MSPLogin(NULL, NULL, login_config);
	if ( ret != MSP_SUCCESS )
	{
		printf("MSPLogin failed , Error code %d.\n",ret);
	}
	//开始一路转写会话
	run_iat("wav/iflytek04.wav" , output_file ,  param1);                      //iflytek04对应的音频内容"一二三四五六七八九十"
	run_iat("wav/iflytek04.wav" , output_file ,  param2);                      //iflytek04对应的音频内容"一二三四五六七八九十"
	//退出登录
        MSPLogout();
	return 0;
}

