#include "docxmask.h"
 std::vector<std::string>  unzipfiles;
 std::vector<std::string> wordrelativepath;

void createDirectory(char *path){

	int posi = 0;
	for (int i = 0; i < strlen(path); i++)
	if (path[i] == '/')
	{
		posi = i;
		char *subpath = new char[posi + 2*sizeof(char)];
		memcpy(subpath, path, posi + sizeof(char));
		subpath[posi + 1] = '\0';
		//cout<<subpath<<endl;
		if (NULL==opendir(subpath))
		{
			 mkdir(subpath, 0775);
		}
		free(subpath);
		subpath = NULL;


	}


}



void zipFolder(char *docx, vector<string> unzipfile)
{
	unsigned int nCount = unzipfile.size();

	zipper zipFile;
	zipFile.open(docx);

	// add files to the zip file
	for (unsigned int i = 0; i < nCount; i++)
	{


		ifstream file(unzipfile[i], ios::in | ios::binary);


		if (file.is_open())
		{
			zipFile.addEntry((char *)wordrelativepath[i].c_str());
			zipFile << file;
		}

		file.close();

	}
	zipFile.close();
}

void unzip(char *docx)
{
	unzipper zipFile;
	zipFile.open(docx);
	auto filenames = zipFile.getFilenames();

	for (auto it = filenames.begin(); it != filenames.end(); it++)
	{

		zipFile.openEntry((*it).c_str());
		int posi = 0;
		for (int i = 0; i < strlen(docx); i++)
		if (docx[i] == '/')
		{
			posi = i;


		}

		char *name = new char[posi + 1 + strlen((*it).c_str()) + 1];
		memset(name, 0, posi + 1 + strlen((*it).c_str()) + 1);
		memcpy(name, docx, posi + 1);
		name[posi + 1] = '\0';
		strcat(name, (*it).c_str());
	//	for (int i = 0; i < strlen(name); i++)
	//	if (name[i] == '/')
	//		name[i] = '\\';




		ofstream file(name, ios::out | ios::binary);
		if (file.is_open()){

			wordrelativepath.push_back((string)(*it).c_str());
			unzipfiles.push_back((string)name);
			zipFile >> file;
			file.close();


		}
		else{

			createDirectory(name);
			it--;


		}

		free(name);
		name = NULL;


	}
}








void deletefile(vector <string> file){
	for (int i = 0; i < file.size(); i++)
	{

		remove(file[i].c_str());

	}
}

void deleteFolder(char *src, vector <string>file){
	int posi = 0;
	for (int i = 0; i < strlen(src); i++)
	if (src[i] == '/')
		posi = i;

	for (int i = 0; i < file.size(); i++)
	{

		char *filename = (char *)file[i].c_str();
		for (int j = file[i].size() - 1; j >= posi; j--){
			if (filename[j] == '/'){
				char *path = new char[j + 1];
				memcpy(path, filename, j);
				path[j] = '\0';
				rmdir(path);
				free(path);
				path = NULL;

			}

		}



	}

}
