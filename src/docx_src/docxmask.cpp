#include "docxmask.h"
#include "mask.h"
using namespace std;

int mask(char *docx1,vector<string> keyword){

	unzip(docx1);

	int flag = masking(docx1,keyword);
	zipFolder(docx1,unzipfiles);

	deleteFolder(docx1, unzipfiles);
	unzipfiles.clear();
	wordrelativepath.clear();





	//system("pause");
	return flag;
}



/*buf for docx in RAM, length for size of docx*/
int docxmask(char **buf, int *length, vector<string> keyword) {
	int pid = getpid();

	if (access("/tmp/docx",F_OK) != 0) {
		if(mkdir("/tmp/docx",0777) != 0) {
			return MKDIR_FAILED;
		}
		system("mount -t tmpfs -o size=3000m tmpfs /tmp/docx");
	}
	
	/*create a folder to save the data from docx*/
	char dir[30];
	sprintf(dir,"/tmp/docx/%d",pid);
	if(mkdir(dir,0777) != 0) {
		std::cerr << "Create the folder failed!" << std::endl;
		return MKDIR_FAILED;
	}


	fstream file_io("/tmp/docx/docx.docx",ios::out | ios::binary | ios::trunc);
	if (!file_io.is_open()) {
		std::cerr << "Please check the file is exist!" << std::endl;
		return OPEN_FILE_FAILED;
	}
	file_io.write(*buf,*length);
	file_io.close();

	/*mask*/
	int flag = mask("/tmp/docx/docx.docx",keyword);

	file_io.open("/tmp/docx/docx.docx",ios::in | ios::binary);
	if (!file_io.is_open()) {
			return OPEN_FILE_FAILED;
	}
	file_io.seekg(0,ios::end);
	long new_length = file_io.tellg();
	file_io.seekg(0,ios::beg);

	if (*length < new_length) {
		delete[] (*buf);
		*buf = new char[new_length];
	}
	*length = new_length;
	file_io.read(*buf,*length);
	file_io.close();

	char rm[50];
	sprintf(rm,"rm -rf /tmp/docx/%d",pid);
	system(rm);

	/*if we can't find any keywords, return 1*/
	if(flag)
		return FIND_NO_KEYWORD;

	return 0;
}