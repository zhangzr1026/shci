
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "../../include/qisr.h"
#include "../../include/msp_cmn.h"
#include "../../include/msp_errors.h"



#define MAX_KEYWORD_LEN 4096
char GrammarID[128];

const char*  get_audio_file(void)
{
	char key = 0;
	while(key != 27)//按Esc则退出
	{
		system("cls");//清屏
		printf("请选择音频文件：\n");
		printf("1.科大讯飞\n");
		printf("2.阿里山龙胆\n");
		printf("3.齐鲁石化\n");
		printf("4.一二三四五六七八九十\n");
		printf("注意：第三条的音频故意结巴说出来的，用于展示效果。\n      关键字列表中没有第四条，展示如果用户说的词语不在列表中，会得到什么结果。\n");
		key = getchar();
		switch(key)
		{
		case '1':
			printf("1.科大讯飞\n");
			return "wav/iflytek01.wav";                               //iflytek01对应的音频内容“科大讯飞”
		case '2':
			printf("2.阿里山龙胆\n");
			return "wav/iflytek02.wav";                               //iflytek02对应的音频内容“阿里山龙胆”
		case '3':
			printf("3.齐鲁石化\n");
			return "wav/iflytek03.wav";                               //iflytek03对应的音频内容“齐鲁石化”
		case '4':
			printf("4.一二三四五六七八九十\n");
			return "wav/iflytek04.wav";                               //iflytek04对应的音频内容"一二三四五六七八九十"
		default:
			continue;
		}
	}
	exit(0);
	return NULL;
}

int get_grammar_id( int upload)
{
	int ret = MSP_SUCCESS;
	const char * sessionID = NULL;
	char UserData[MAX_KEYWORD_LEN];
	unsigned int len = 0;
	const char* testID = NULL;
	FILE *fp = NULL;
	memset(UserData, 0, MAX_KEYWORD_LEN);
	if (0 == upload)

	{
		strcpy(GrammarID, "e7eb1a443ee143d5e7ac52cb794810fe");
		                 //65bc94965a076ecaf6ea79899bebc534
		//这个ID是我上传之后记录下来的。语法上传之后永久保存在服务器上，所以不要反复上传同样的语法
		return 0;
	}	
	//如果想要重新上传语法，传入参数upload置为TRUE，就可以走下面的上传语法流程
	fp = fopen("asr_keywords_utf8.txt", "rb");//关键字列表文件必须是utf8格式
	if (fp == NULL)
	{
		printf("keyword file cannot open\n");
		return -1;
	}
	len = (unsigned int)fread(UserData, 1, MAX_KEYWORD_LEN, fp);
	UserData[len] = 0;
	fclose(fp);
    testID = MSPUploadData("userword", UserData, len, "dtt = userword, sub = asr", &ret);// sub 参数必需，否则返回空串
	if(ret != MSP_SUCCESS)
	{
		printf("UploadData with errorCode: %d \n", ret);
		return ret;
	}
	memcpy((void*)GrammarID, testID, strlen(testID));
	printf("*************************************************************\n");
	printf("GrammarID: \"%s\" \n", GrammarID);//将获得的GrammarID输出到屏幕上
	printf("*************************************************************\n");
	return 0;
}

int run_asr(const char* asrfile ,  const char* param)
{
	char rec_result[1024*4] = {0};
	const char *sessionID;
	FILE *f_pcm = NULL;
	char *pPCM = NULL;
	int lastAudio = 0 ;
	int audStat = 2 ;
	int epStatus = 0;
	int recStatus = 0 ;
	long pcmCount = 0;
	long pcmSize = 0;
	int ret = 0 ;
	sessionID = QISRSessionBegin(GrammarID, param, &ret); //asr
	if(ret !=0)
	{
		printf("QISRSessionBegin Failed,ret=%d\n",ret);
	}

	f_pcm = fopen(asrfile, "rb");
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
		}
		usleep(150000);
	}
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
	const char* login_config = "appid = 53be54f9,work_dir =   .  ";
	const char* param = "rst=plain,rse=gb2312,sub=asr,ssm=1,aue=speex;7,auf=audio/L16;rate=16000,ent=sms16k";    //注意sub=asr
	int ret = 0 ;
	char key = 0 ;
	int grammar_flag = 1 ;//0:不上传词表；1：上传词表
	const char* asrfile = get_audio_file();//选择
	ret = MSPLogin(NULL, NULL, login_config);
	if ( ret != MSP_SUCCESS )
	{
		printf("MSPLogin failed , Error code %d.\n",ret);
		return 0 ;
	}
	memset(GrammarID, 0, sizeof(GrammarID));
	ret = get_grammar_id(grammar_flag);
	if(ret != MSP_SUCCESS)
	{
		printf("get_grammar_id with errorCode: %d \n", ret);
		return 0 ;
	}
 	ret = run_asr(asrfile, param);
	if(ret != MSP_SUCCESS)
	{
		printf("run_asr with errorCode: %d \n", ret);
		return 0;
	}
	MSPLogout();
	return 0;
}

