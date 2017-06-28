// realTimeSdk.cpp : Defines the entry point for the console application.
//

#include <string>
#include <iostream>
#include "Nls.h"
#include "openssl/crypto.h"
#include "openssl/err.h"
#include "openssl/ssl.h"
#include <vector>
#include <fstream>
#include <thread>
#include <mutex>
#include <sstream>


using namespace std;
#define FRAME_SIZE 640
#define MUTEX_TYPE       std::mutex*
#define MUTEX_SETUP(x)   x = new std::mutex()
#define MUTEX_CLEANUP(x) delete x
#define MUTEX_LOCK(x)    x->lock()
#define MUTEX_UNLOCK(x)  x->unlock()
#define THREAD_ID        this_thread::get_id()



void handle_error(const char *file, int lineno, const char *msg)
{
	fprintf(stdout, "** %s:%d %s\n", file, lineno, msg);
	/* exit(-1); */
}

/* This array will store all of the mutexes available to OpenSSL. */
static MUTEX_TYPE *mutex_buf = NULL;

static void locking_function(int mode, int n, const char *file, int line)
{
	if (mode & CRYPTO_LOCK)
		MUTEX_LOCK(mutex_buf[n]);
	else
		MUTEX_UNLOCK(mutex_buf[n]);
}

static unsigned long id_function(void)
{
	ostringstream os;
	os << this_thread::get_id();
	unsigned long id = 0;
	istringstream in(os.str());
	in >> id;
	return ((unsigned long)id);
}

int thread_setup(void)
{
	int i;

	mutex_buf = (std::mutex**)malloc(CRYPTO_num_locks() * sizeof(MUTEX_TYPE));
	if (!mutex_buf)
		return 0;
	for (i = 0; i < CRYPTO_num_locks(); i++)
		MUTEX_SETUP(mutex_buf[i]);
	CRYPTO_set_id_callback(id_function);
	CRYPTO_set_locking_callback(locking_function);
	return 1;
}

int thread_cleanup(void)
{
	int i;

	if (!mutex_buf)
		return 0;
	CRYPTO_set_id_callback(NULL);
	CRYPTO_set_locking_callback(NULL);
	for (i = 0; i < CRYPTO_num_locks(); i++)
		MUTEX_CLEANUP(mutex_buf[i]);
	free(mutex_buf);
	mutex_buf = NULL;
	return 1;
}


int func(string testfile)
{
	try
	{
		int port = 443;
		string ip = "nls-trans.dataapi.aliyun.com";
		string id = "";//  id 
		string scret = "";// scret  
		string url = "wss://" + ip + ":" + to_string(port) + "/realtime";
		Nls nls(url, 10); 
		nls.setSampleRate(8000);
		nls.setResponseMode(0);
		nls.setApp_key("");		 //key
		nls.authorize(id, scret);
		//nls.setVocaId("3");
		//nls.setUserId("");
		//nls.setFormat("pcm");	 //
		nls._onResultReceivedEvent = ([&](string str)
		{
			cout << str << endl;
		});
		if (nls.startNls() == false)
		{
			return 0;
		}
		ifstream fs;
		fs.open(testfile.c_str(), ios::binary | ios::in);
		if (!fs)
		{
			throw std::runtime_error("file isn't exist");
		}
		//fs.seekg(44);
		while (!fs.eof())
		{
			vector<char> data(FRAME_SIZE, 0);
			fs.read(&data[0], sizeof(char) * FRAME_SIZE);
			int nlen = fs.gcount();
			nls.sendAsr((char*)&data[0], nlen);
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
		fs.close();
		nls.closeNls();
	}
	catch (exception e)
	{
		cout << e.what() << endl;
	}
	return 0;
}

int main(int arc, char* argv[])
{
	thread_setup();
	const int ThreadCount = 10;
	std::thread tarr[ThreadCount];
	for (int i = 0; i < 100; i++)
	{
		for (int j = 0; j < ThreadCount; j++)
			tarr[j] = std::thread(func, argv[1]);
		for (int j = 0; j < ThreadCount; j++)
			tarr[j].join();
	}
	thread_cleanup();

	return 0;
}

