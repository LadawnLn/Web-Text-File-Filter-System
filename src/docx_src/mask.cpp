#include <vector>
#include "mask.h"
#include <string>
#include<stdio.h>
#include<sstream>
#include<stdlib.h>
int P[100];

string int_to_string(int i){
	char buffer[sizeof(int)];
	sprintf(buffer,"%d",i);
	string s(buffer);
	return s;
}





char* KMP(char *A, char *B, int size)
{
	InitP(B);
	int len1 = size, len2 = strlen(B);
	int i, j = 0;                  
	for (i = 0; i<len1; ++i)
	{
		while (j>0 && A[i] != B[j])   
			j = P[j];


		if (A[i] == B[j])
			++j;
		if (j == len2)              
			return A + i - j + 1;       
	}
	return NULL;
}

void InitP(char *B)
{
	P[0] = 0;
	P[1] = 0;
	int i, j = 0, len = strlen(B);
	for (i = 2; i<len; ++i)
	{
		while (j>0 && B[j] != B[i - 1])  
			j = P[j];
		if (B[j] == B[i - 1])
			++j;
		P[i] = j;
	}
}


int subfile_filter(char *path, char *filename, vector<string> keyword , bool *find_keyword, int flag){
	// *find_keyword = false;
	int posi = 0;
	for (int i = 0; i < strlen(path); i++) {
		if (path[i] == '/')
			posi = i;
	}

	char *document = new char[strlen(filename) + posi + sizeof(char)];
	memset(document, 0, strlen(filename) + posi + sizeof(char));
	memcpy(document, path, posi );
	document[posi + sizeof(char)] = '\0';
	strcat(document, filename);

	fstream is_file_exist(document, ios::in);
	if (is_file_exist.is_open()){
		fstream file(document, ios::out | ios::binary | ios::in);
		if (file.is_open()){
			file.seekp(0, ios::end);
			long int size = file.tellp();
			long int document_len = size;
			file.seekp(0, ios::beg);
			char *txt = new char[size];
			char *content = new char[size];
			memset(txt, 0, size);
			memset(content, 0, size);
			long int *contentLen = new long int[1];
			file.read(txt, size);
			*contentLen = 0;

			vector<vector<long int> > record_position = extract_content(txt, size, content, contentLen, flag);

			for(int i = 0; i < keyword.size(); ++i) {
				char *word = (char*)keyword[i].c_str();

							size = *contentLen;
							char *head = NULL;
							char *tmp = content;


							while (size > 0){

								head = KMP(tmp, word, size);
								if (head != NULL){
									*find_keyword = true;
									size = *contentLen - (head - content) - strlen(word);
									tmp = content + (head - content) + strlen(word);
									for (int i = 0; i < strlen(word); i++)
										head[i] = '*';
								}
								else
									break;


							}
			}

			long int before = 0;
			for (int i = 0; i < record_position.size(); i++){
				memcpy(txt + record_position[i][1], (const char *)content + before, record_position[i][0] - before);
				before = record_position[i][0];
			}

			file.seekg(0, ios::beg);
			file.write(txt, document_len);
			file.close();
			delete(contentLen);
			delete(txt);
			delete(content);
			is_file_exist.close();
			return 1;


		}
		else{
			cout << "未找到关键字！" << endl;
			is_file_exist.close();
			return 2;
		}
	}
	else
		return 0;




}

//单独处理SmartArt
int SmartArtfilter(char *path, char *filename, vector<string> word){

	int posi = 0;
	for (int i = 0; i < strlen(path); i++){
		if (path[i] == '/')
			posi = i;
	}
	char *pa = new char[posi+sizeof(char)];
	memcpy(pa,path,posi);
	pa[posi] = '\0';
	string p = pa;
	string f = filename;
	string document = p + f;

	fstream is_file_exist(document, ios::in);
	if (is_file_exist.is_open()){
		fstream file(document, ios::out | ios::binary | ios::in);
		file.seekp(0, ios::end);
		long int size = file.tellp();
		long int document_len = size;
		file.seekp(0, ios::beg);
		char *txt = new char[size];
		memset(txt, 0, size);
		file.read(txt, size);

		char *begin = "<a:t";
		char *end = "</a:t>";
		char *tmp = txt;

		while (size > 0){
			//find the content
			char *head = KMP(tmp, begin, size);
			char *tail = KMP(tmp, end, size);
			int i = 0;
			if (head == NULL)
				break;
			for (i = strlen(begin); head[i] != '>'; i++);
			head = head + i+sizeof(char);
			int len = tail - head ;
			char *isword = NULL;
			for(int i = 0; i < word.size(); ++i) {
				while(len>0){
					char *wword = (char*)word[i].c_str();
								isword = KMP(head , wword, len);
								if (isword != NULL){
									for (int j = 0; j < strlen(wword); j++)
										isword[j] = '*';
									head = isword + strlen(wword);
									len = len - (isword - head) - strlen(wword);

								}else
									break;
							}
							size = size - (tail- txt) - strlen(end);
							tmp = tail + strlen(end);
						}
			}
		file.seekg(0, ios::beg);
		file.write(txt, document_len);
		file.close();
		delete(txt);
		delete(pa);
		return 1;
	}
	else
		return 0;
}


int masking(char *path, vector<string> word){


	bool find_keyword = false;
	char *document[] = { "/word/document.xml", "/word/comments.xml", "/word/footnotes.xml", "/word/endnotes.xml" };
	char *docprops[] = {"/docProps/app.xml","/docProps/core.xml" };
	int count = sizeof(document) / sizeof(char *);
	for (int i = 0; i < count; ++i){
		subfile_filter(path, document[i], word, &find_keyword, 0);
	}
	for(int i = 0; i < 2; ++i) {
		subfile_filter(path, docprops[i], word, &find_keyword, 1);
	}
	int i = 1;
	string header = "/word/header" + int_to_string(i) + ".xml";
	while (subfile_filter(path, (char *)header.c_str(), word, &find_keyword, 0)){
		i++;
		header = "/word/header" + int_to_string(i) + ".xml";
	}
	
	i = 1;
	string footer = "/word/footer" + int_to_string(i) + ".xml";
	while (subfile_filter(path, (char *)footer.c_str(), word, &find_keyword, 0)){
		i++;
		footer = "/word/footer" + int_to_string(i) + ".xml";
	}

	//处理SmartArt
		i = 1;
		string smartart = "/word/diagrams/data" + int_to_string(i) + ".xml";
		while (SmartArtfilter(path, (char *)smartart.c_str(), word)){
			i++;
			smartart = "/word/diagrams/data" + int_to_string(i) + ".xml";
		}


		if(find_keyword)
			return 0;
		else
			return 1;




}


vector<vector<long int>> extract_content(char *txt, int size, char *content, long int *contentLen,int flag) {
	vector<vector<long int>> record_position;
	const char *begin = "<w:t";
	const char *end = "</w:t>";
	const char *tab = "<w:tab";
	const char *deltxthead = "<w:delText"; 
	const char *deltxtend = "</w:delText>";
	/*Abstract Information*/
	const char *abstractHead[] = {"<dc:title>", "<dc:subject>", "<cp:keywords>", "<dc:description>", 
		"<cp:lastModifiedBy>", "<dc:creator>", "<Company>", "<cp:category>", "<Manager>", "<Application>"};
	const char *abstractEnd[] = {"</dc:title>", "</dc:subject>", "</cp:keywords>", "</dc:description>", 
		"</cp:lastModifiedBy>", "</dc:creator>", "</Company>", "</cp:category>", "</Manager>", "</Application>"};

	int count = 0;

	char *tmp = txt;
	while (size > 0){
		
		if (flag == 0) {
			// Process the document
			char *head = KMP(tmp, (char *)begin, size);
			char *tail = KMP(tmp, (char *)end, size);
			char *Tab = KMP(tmp, (char *)tab, size);
			char *delhead = KMP(tmp, (char *)deltxthead, size);
			char *deltail = KMP(tmp, (char *)deltxtend, size);
			if ((head == NULL || tail == NULL) && (delhead == NULL || deltail == NULL)) {
				break;
			}
			else {
				if (*(head + strlen(begin)) != '>' && *(head + strlen(begin)) != ' '){
					size = size - (head-tmp) -strlen(begin);
					tmp = head + strlen(begin);
					continue;
				}
			}
			if (head < delhead){
				if (head != NULL&&delhead != NULL){
					int i = 0;
					for (i = strlen(begin); head[i] != '>'; i++);
					head = head + i + 1;
					memcpy(content + *contentLen, head, tail - head);
					*contentLen = *contentLen + (tail - head);
					record_position.resize(count + 1);
					record_position[count].resize(2);
					record_position[count][0] = *contentLen;
					record_position[count][1] = head - txt;
					size = size - (tail - tmp) - strlen(end);
					tmp = tail + strlen(end);
					count++;
				}
				else {
					int i = 0;
					for (i = strlen(deltxthead); delhead[i] != '>'; i++);
					delhead = delhead + i + 1;
					memcpy(content + *contentLen, delhead, deltail - delhead);
					*contentLen = *contentLen + (deltail - delhead);
					record_position.resize(count + 1);
					record_position[count].resize(2);
					record_position[count][0] = *contentLen;
					record_position[count][1] = delhead - txt;
					size = size - (deltail - tmp) - strlen(end);
					tmp = deltail + strlen(deltxtend);
					count++;
				}

			}

			if (head > delhead) {
				if (delhead == NULL){
					int i = 0;
					for (i = strlen(begin); head[i] != '>'; i++);

					head = head + i + 1;
					//		cout << head << endl;
					memcpy(content + *contentLen, head, tail - head);
					*contentLen = *contentLen + (tail - head);
					record_position.resize(count + 1);
					record_position[count].resize(2);
					record_position[count][0] = *contentLen;
					record_position[count][1] = head - txt;
					size = size - (tail - tmp) - strlen(end);
					tmp = tail + strlen(end);
					count++;
				}
				else{
					int i = 0;
					for (i = strlen(deltxthead); delhead[i] != '>'; i++);
					delhead = delhead + i + 1;
					memcpy(content + *contentLen, delhead, deltail - delhead);
					*contentLen = *contentLen + (deltail - delhead);
					record_position.resize(count + 1);
					record_position[count].resize(2);
					record_position[count][0] = *contentLen;
					record_position[count][1] = delhead - txt;
					size = size - (deltail - tmp) - strlen(end);
					tmp = deltail + strlen(deltxtend);
					count++;
				}
			}
		
		}
		
		else {
			// Process the abstract
			vector<char *> head, end;
			int num = sizeof(abstractHead) / sizeof(char *);
			for(int i = 0; i < num; ++i) {
				char *absHead = KMP(tmp, (char *)abstractHead[i], size);
				char *absEnd = KMP(tmp,(char *)(abstractEnd[i]), size);
				head.push_back(absHead);
				end.push_back(absEnd);
			}
			for(int i = 0; i < num; ++i) {
				if (head[i] == NULL) {
					continue;
				}
				head[i] = head[i] + strlen(abstractHead[i]);
				memcpy(content + *contentLen, head[i], end[i] - head[i]);
				*contentLen = *contentLen + (end[i] - head[i]);
				record_position.resize(count + 1);
				record_position[count].resize(2);
				record_position[count][0] = *contentLen;
				record_position[count][1] = head[i] - txt;
				count++;
			}
			break;
		}

	}
	content[*contentLen] = '\0';
	return record_position;
}
