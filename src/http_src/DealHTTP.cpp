#include <iostream>
#include <string.h>
#include <vector>
#include "docxmask.h"

/*********************************************************
    作用：将整形变量value转化为字符串，放入result指向内存。
    参数：
        value：正整数或负整数
        base：转化的进制数，选择范围为[2,36]
        result：指向一段内存区域来存放value的转化值，空间如果不足够则会导致指针越界操作
*********************************************************/
void itoa(int value, char* result, int base) 
{
		// check that the base if valid
		if (base < 2 || base > 36) { *result = '\0'; return; }

		char* ptr = result, *ptr1 = result, tmp_char;
		int tmp_value;

		do {
			tmp_value = value;
			value /= base;
			*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
		} while ( value );

		// Apply negative sign
		if (tmp_value < 0) *ptr++ = '-';
		*ptr-- = '\0';
		while(ptr1 < ptr) {
			tmp_char = *ptr;
			*ptr--= *ptr1;
			*ptr1++ = tmp_char;
		}
		return;
}


/* 将string 切分成 vector<string> */
int splitStringToVect(const string & srcStr, vector<string> & destVect, const string & Keyword)
{
    string Tempstr;         //临时存储当前子串
    int Pos = 0;            //当前子串的第一个字符相对于srcStr的偏移值
    int Templen;            //当前子串的长度
    while(1)
    {
        Templen = srcStr.find(Keyword, Pos) - Pos;      //find返回的相对值是对应于整个string的
        if (Templen < 0)
            break;
        Tempstr = srcStr.substr(Pos, Templen);          //将当前找到的子串放入Tempstr中
        if(Tempstr.length() > 0)                        //如果Tempstr是空字符串则不放入destVect中
            destVect.push_back(Tempstr);
        Pos += Templen + Keyword.length();              //Pos移动到下一个子串的起始位置
    }
    Tempstr = srcStr.substr(Pos, srcStr.length() - Pos);//最后一个子串
    if(Tempstr.length() > 0)
        destVect.push_back(Tempstr);
    return destVect.size();
}


/***********************************************************************************
    函数参数：
        buffer：传入的pdf文件的首部指针
        filelen：传入的pdf文件的大小
        keyword：指定的脱敏关键词
    函数功能：
        pdf文本过滤处理，滤除pdf中的指定关键词
************************************************************************************/
int pdf(char *buffer, long filelen, const std::vector<std::string> &keyword)
{

    StreamParser streamparser;
    int err = streamparser.ParsePdf(buffer,filelen,keyword);
    return err;
}


/***********************************************************************************
    函数参数：
        buf：HTTP报文中Content-Length后的数据。
    返回值：
        Content-Length的int类型数值。
    函数功能：
        取得HTTP消息头中Content-Length后面的长度值。
************************************************************************************/
int GetLength(const char * const buf)
{
    const char * PtoLength = buf;
    char S_DataLen[100];
    int DataLen=0;
    if(PtoLength != NULL)
    {
        int i = 0;
        while(*PtoLength != '\r')
        {
            S_DataLen[i++] = *PtoLength;
            PtoLength++;
        }
        S_DataLen[i] = '\0';
        DataLen = atoi(S_DataLen);
    }
    return DataLen;
}


/***********************************************************************************
    函数参数：
        buf：HTTP报文中Content-Length：后的数据。
        NewLen：将要更新的长度值。
    函数功能：
        更新docx文件的HTTP报文中的Content-Length为过滤后的docx文件大小
************************************************************************************/
void UpdataLen(char *buf, int NewLen)
{
    char * PtoLength = buf;
    char newlen[20]={0};
    itoa(NewLen,newlen,10);
    if(PtoLength != NULL)
    {
        int j=0;
        while(*PtoLength != '\r')
        {
            *PtoLength = newlen[j];
            PtoLength++;
            j++;
        }
    }
}


/************************************************************************************
    函数参数：
        buf：载有docx文件的HTTP报文
        bufsize：HTTP报文的大小
        data：该HTTP报文中的docx数据部分
        datalength：docx数据部分的大小
    返回值：
        tuoming_docx()函数的返回值，即docx文件敏感词过滤成败结果。
    函数功能：
        对docx文件进行过滤，并且将新生成的newbuf作为新的响应数据发回到客户端。
    内部变量说明：
        tmp表示docx文件的大小，将其地址作为形参传入ldt()函数，并规定：
            当docx过滤成功时，将tmp改为新的docx文件的大小；
            当docx过滤失败时，将tmp值设置为0。
        newbuf表示新生成的buf（包括HTTP头部和新的docx文件）
*************************************************************************************/
int dealdocx(char **buf,int *bufsize,char *data, int datalength,char **bufRes, vector<string> keyword)
{
	//单个http的情况
	if(*bufRes == NULL)
	{
		int HeadLen = *bufsize-datalength;
	    char *p_data = new char[datalength];
	    for(int i=0; i<datalength; i++)
	        *(p_data+i) = *(data+i);
	    int tmp = datalength;
	    int docx_flag = docxmask(&p_data,&tmp,keyword);
	    char *newbuf = new char[HeadLen+tmp];
	    char *PtoLen = NULL;
	    //进行内存移植(http头 + 新的docx文件)
	    int i=0;
	    for(; i<HeadLen; i++)
	        *(newbuf+i) = *(*buf+i);
	    for(int j=0; j<tmp; j++)
	        *(newbuf+i+j) = *(p_data+j);
	    if((PtoLen = KMP(newbuf, const_cast<char *>("Content-Length: "),HeadLen,16)) != NULL)
	    {
	        UpdataLen(PtoLen+16,tmp);
	    }
	    //改变bufsize的真实大小值
	    *bufsize = HeadLen+tmp;
	    //改变buf的指向为newbuf
	    delete[] *buf;
	    *buf = newbuf;
	    //释放存放过滤后docx的内存块
	    delete[] p_data;
	    p_data = NULL;
	    return docx_flag;
	}
	//多个http的情况
	else
	{
		int HeadLen = (*bufRes-*buf)-datalength;
		int docxResLen = *bufsize - (*bufRes-*buf);
	    char *p_data = new char[datalength];
	    for(int i=0; i<datalength; i++)
	        *(p_data+i) = *(data+i);
	    int tmp = datalength;
	    /**************ldt函数用于处理docx的脱敏**************/	
	    int docx_flag = docxmask(&p_data,&tmp,keyword);
	    char *newbuf = new char[HeadLen+tmp+docxResLen];
	    char *PtoLen = NULL;
	    //进行内存移植(http头 + 新的docx文件)
	    int i=0;
	    for(; i<HeadLen; ++i)
	        *(newbuf+i) = *(*buf+i);
	    int j=0;
	    for(; j<tmp; ++j)
	        *(newbuf+i+j) = *(p_data+j);
	    for(int k=0; k<docxResLen; ++k)
	    	 *(newbuf+i+j+k) = *(*bufRes+k);
	    if((PtoLen = KMP(newbuf, const_cast<char *>("Content-Length: "),HeadLen,16)) != NULL)
	    {
	        UpdataLen(PtoLen+16,tmp);
	    }
	    //释放原始buf，改变buf的指向为newbuf
	    delete[] *buf;
	    *buf = newbuf;
	    //改变bufRes的指向
	    *bufRes = newbuf+HeadLen+tmp;
	    //改变bufsize的真实大小值
	    *bufsize = HeadLen+tmp+docxResLen;
	    //释放存放过滤后docx的内存块
	    delete[] p_data;
	    p_data = NULL;
	    //返回ldt函数的返回值
	    return docx_flag;
	}

}


/***********************************************************************************
    函数参数：
        buf：将要处理的html流
        bufsieze： buf的长度
        keyword： 将要过滤的关键词字符串
        keysize： keyword的长度
    函数功能：
        对html流缓存区buf中关键词进行过滤，将关键词改为"****".
************************************************************************************/
int html(char *buf, int bufsize, char *keyword,int keysize)
{
    char *start =NULL;
    char *Buf = buf;
    char *Key = keyword;
    int Bufsize = bufsize;
    int Keysize = keysize;
    int keyword_count = 0;
    while( (start =  KMP(Buf, Key, Bufsize, keysize) ) != NULL)
    {
        char *temp = start;
        for(int i=0; i<keysize; i++)
        {
            *(temp+i) = '*';
        }
        Buf = start + keysize;
        Bufsize = bufsize - (start-buf);
        keyword_count++;
    }
    return keyword_count;
}



/**********************************************************************************************************
    函数参数：
        buf：服务端传回的http包报文字符串
        bufsieze： buf的长度
        bufRes ：若buf中有多个http报文，则代表buf中第二个http报文的头部指针；若buf中没有第二个http报文，其值为NULL
        keyword： 脱敏关键词
        host：主机名
    函数返回值：
        -1:表示buf中的http报文消息头不完整，若发生这种情况，则直接丢掉buf数据重新接收
        0：表示buf中第一个http报文是文件形式，并且没有接收完整
        1：表示buf中只有一个http报文，并且接收完整
        2：表示buf中有多个http报文，第一个接收完整，第二次http报文头部指针需要返回
        3：表示buf中的http报文响应不是200 OK，而是304、100等等
    函数功能：
        对服务器传回的http包数据进行类型判断，对其中的html、docx文件过滤
************************************************************************************************************/
int parseHttp(char **buf,int *bufsize,char **bufRes, std::string keyword, std::string host)
{

    char *start = NULL;
    char *end = NULL;
    int Flag_Pdf=0;
    int Flag_doc=0;
    int Flag_docx=0;
    std::vector<std::string>  KeyWord_word ;
    splitStringToVect(keyword, KeyWord_word, " " ) ;
    //以HTTP开头的数据，默认为一个HTTP报文
    if( strncmp(*buf,"HTTP",4) == 0 )
    {
        //取得HTTP报文的消息头信息
        if( ((start = KMP(*buf, const_cast<char *>("HTTP/1."),*bufsize,7)) != NULL) && ( (end = KMP(*buf, const_cast<char *>("\r\n\r\n"),*bufsize,4)) != NULL) )
        {
            char *PtoContentType = NULL;
            //取得HTTP报文消息头中包含的文件类型信息
            if( (PtoContentType = KMP(start, const_cast<char *>("Content-Type: "),end-start,14)) != NULL)
            {
                char *PtoType = NULL;
                //判断HTTP报文的文件类型为html
                if( ((PtoType = KMP(PtoContentType, const_cast<char *>("text/html"),end-PtoContentType,9)) != NULL) )
                {
                    char *PtoNextHttp = NULL;
                    char *PtoData = end+4;
                    int keyword_find_count = 0;
                    //该buf中包含有多个HTTP报文，其中第一个为html类型
                    if( (PtoNextHttp = KMP(end, const_cast<char *>("HTTP/1."),*bufsize-(end-start),7)) != NULL )
                    {
                        //对第一个HTTP报文进行html文件的过滤处理
                        for(int i=0; i < KeyWord_word.size(); i++)
                        {
                            if(html(PtoData,PtoNextHttp-PtoData,const_cast<char *>(KeyWord_word[i].data()),KeyWord_word[i].size())>0)
                                keyword_find_count++;
                        }
                        //将之后的HTTP报文数据发回缓冲区
                        *bufRes = PtoNextHttp;
                        return 2;                           //After bufRes send    ; Before bufRes return buf
                    }
                    //该buf中只含有一个HTTP报文
                    else
                    {
                        //对该HTTP报文进行html过滤处理
                        for(int i=0; i < KeyWord_word.size(); i++)
                        {
                            if(html(PtoData,*bufsize-(PtoData-start),const_cast<char*>(KeyWord_word[i].data()),KeyWord_word[i].size())>0)
                                keyword_find_count++;
                        }
                        *bufRes = NULL;
                        return 1;                       //send all buf
                    }
                }
                //判断该HTTP报文为docx文件类型
                else if ( ((PtoType = KMP(PtoContentType, const_cast<char *>("application/vnd.openxmlformats-officedocument.wordprocessingml.document"),end-PtoContentType,60)) != NULL) )
                {
                    char *PtoDocxLength = NULL;
                    char *PtoDocxData = NULL;
                    //取得docx文件的长度
                    if( ((PtoDocxLength=KMP(*buf,const_cast<char *>("Content-Length: "),end-start,16)) != NULL) )
                    {
                        PtoDocxLength += 16;
                        PtoDocxData = end+4;
                        int Docxlength = GetLength(PtoDocxLength);
                        char *DocxHttpTemp = KMP(PtoDocxData,const_cast<char *>("HTTP/1."),*bufsize-(PtoDocxData-*buf),7);
                        //该buf中不只有一个http报文
                        if(DocxHttpTemp != NULL)
                        {
                            //判断该docx文件为完整
                            if(Docxlength == DocxHttpTemp-PtoDocxData)
                            {
                                //对docx文件进行过滤处理
                                *bufRes = DocxHttpTemp;
                                int New_bufsize = *bufsize; 
                                dealdocx(buf,&New_bufsize,PtoDocxData,Docxlength,bufRes,KeyWord_word);
                                *bufsize = New_bufsize;
                                return 2;           //After bufRes send    ; Before bufRes return buf
                            }
                            //docx文件不完整，不可能发生
                            else
                            {
                                *bufRes = *buf;
                                return 0;
                            }
                        }
                        //该buf中只有一个http报文
                        else
                        {
                            //判断该docx文件完整
                            if(Docxlength == *bufsize - (PtoDocxData-*buf))
                            {
                                //对docx文件进行过滤处理
                                *bufRes = NULL;
                                int New_bufsize = *bufsize;             
                                dealdocx(buf,&New_bufsize,PtoDocxData,Docxlength,bufRes,KeyWord_word); 
                                *bufsize = New_bufsize;
                                return 1;       //send all Docx buf
                            }
                            //该docx文件不完整，返回0，将buf整体发回缓冲区
                            else
                            {
                                *bufRes = *buf;
                                return 0;
                            }
                        }
                    }
                    else
                    {
                        std::cout<<"Can not find Docx Content-Length\n";
                    }
                }
                //对于不属于html、pdf、doc、docx的其他HTTP文件类型，将他们不处理直接发送给client
                else
                {
                    //not file and stream style ,then send it directly.
                    *bufRes = NULL;
                    return 3;
                }
            }
            //对于不包含Content-Type的HTTP报文，对其不处理直接发送到client
            else
            {
                return 3;
                std::cerr<<"the http headers has not Content-Type\n";
            }
        }
        //http报文的消息头不完整，这种情况若发生，则直接将buf丢掉
        else
        {
            return -1;
            std::cerr<<"the http headers is not complete\n";
        }
    }
    //该buf不是以"HTTP /1."开头，此时该buf可能为html文件的数据部分，也可能是非HTTP类型的TCP报文，此时将该buf中文本数据进行过滤，达到流过滤效果。
    else
    {
        char *PtoHeader = NULL;
        int keyword_find_count = 0;
        //该buf中有其他的http报文
        if( (PtoHeader=KMP(*buf,const_cast<char *>("HTTP/1."),*bufsize,7)) !=NULL)
        {
            //对该buf中第二个http报文之前的数据进行html过滤处理
            for(int i=0; i < KeyWord_word.size(); i++)
            {
                if(html(*buf,*bufsize-(PtoHeader-*buf),const_cast<char *>(KeyWord_word[i].data()),KeyWord_word[i].size()) >0)
                    keyword_find_count++;
            }
            //将第一个HTTP报文之后的数据发回缓冲区
            *bufRes = PtoHeader;
            return 2;
        }
        //给buf中只有一块纯数据部分
        else
        {
            //对该buf中的数据进行html过滤处理
            for(int i=0; i < KeyWord_word.size(); i++)
            {
                if(html(*buf,*bufsize,const_cast<char *>(KeyWord_word[i].data()),KeyWord_word[i].size())>0)
                    keyword_find_count++;
            }
            *bufRes = NULL;
            return 1;                       
        }
    }
}

