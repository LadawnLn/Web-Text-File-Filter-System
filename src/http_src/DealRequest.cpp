#include <iostream>
#include <string.h>

/*
函数参数：	
	buf： 要处理的字符串
	bufsize： buf的长度
	bufDel： 要删去的字符串
函数返回值：
	返回在 buf 里面找到的 bufDel 的首地址 
函数功能：
	删除字符串中的指定子串。 
*/

char * DelWords(char *buf,int bufsize,const char *bufDel)
{
	int i=0;
	int j=0;
	int k=0;
	int DelSize = strlen(bufDel);
	for(i=0;i<bufsize;++i)
	{
		if(buf[i] == bufDel[0])
		{
			k=i+1;
			for(j=1;buf[k]==bufDel[j] && j<DelSize && k<bufsize;j++,k++);
			if(j == DelSize)
				return buf+i;
		}
	}
	return NULL;
}

/*
函数参数：	
	buf： 要处理的字符串
	bufsize： buf的长度
	bufDel： 要删去的字符串
函数返回值：
	返回删除子串 bufDel后 buf 的长度。 当header头部不完整时，返回-1，表示丢弃此 buf.
函数功能：
	处理 HTTP 的请求报文，删去指定的 bufDel 的子串，使得服务端发送响应不是206而是200. 
*/

int parseRequest(char *buf,int bufsize,const char * bufDel,char flag)
{
	char *PtoDelstart=NULL;
	int DelSize = strlen(bufDel);
	int Endbufsize = bufsize;
	while( (PtoDelstart=DelWords(buf, Endbufsize,bufDel)) != NULL)
	{
		char *Ptoend = NULL;
		char *Ptoifrange = NULL;
		char *PtoDelend = NULL;
		if( (Ptoend = KMP(PtoDelstart,"\r\n\r\n",bufsize-(PtoDelstart-buf),4)) != NULL)
		{
			Ptoend += 4;
			if(flag == 2)
			{
        		if( (Ptoifrange = KMP(PtoDelstart,"If-Range",Ptoend-PtoDelstart,8)) !=NULL)
        		{
        			PtoDelend = KMP(Ptoifrange,"\r\n",Ptoend-Ptoifrange,2) + 2;
        		}
        		else
        		{
                    PtoDelend = KMP(PtoDelstart,"\r\n",Ptoend-PtoDelstart,2) + 2;
        		}
		    }
			else if(flag == 1)
			{
				PtoDelend = KMP(PtoDelstart,"\r\n",Ptoend-PtoDelstart,2) + 2;
			}
		}
		else
		{
			std::cerr<<"can not find end\n";
			return -1;
		}
		
		for(int n=0; n<Endbufsize-(PtoDelstart-buf); n++)
		{
			*(PtoDelstart+n) = *(PtoDelend+n);
		}
		Endbufsize = Endbufsize - (PtoDelend-PtoDelstart);
		PtoDelstart = NULL; 
	}
	return Endbufsize;
}

