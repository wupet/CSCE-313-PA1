/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Peter Wu 
	UIN: 135007249 
	Date: 9/16/2025
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <sys/wait.h>
#include <chrono>


using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	int p = -1;
	double t = -1;
	int e = 0;
    int m = MAX_MESSAGE;
    bool new_chan = false;
    vector <FIFORequestChannel*> channels;
	
	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) { //grabs the arguments
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
            case 'm':
                m = atoi (optarg);
                break;
            case 'c':
                new_chan = true;
                break;
		}
	}

    pid_t pid = fork();
    if (pid < 0) {
        cerr << "Fork failed\n";
        exit(1);
    }
    else if (pid == 0) {
        // Child -> run server
        char* args[] = { (char*)"server", (char*)"-m", (char*) to_string(m).c_str(), NULL };
        execvp("./server", args);

        cerr << "Exec failed\n";
        exit(1);
    }

    FIFORequestChannel cont_chan("control", FIFORequestChannel::CLIENT_SIDE);
    channels.push_back(&cont_chan);
    char buf[MAX_MESSAGE]; // 256
    if(new_chan)
    {
        MESSAGE_TYPE nc = NEWCHANNEL_MSG;
        cont_chan.cwrite(&nc, sizeof(MESSAGE_TYPE));
        char chan_name[MAX_MESSAGE];
        memset(chan_name, 0, MAX_MESSAGE);
        cont_chan.cread(&chan_name, MAX_MESSAGE);
        FIFORequestChannel* new_chan = new FIFORequestChannel(chan_name, FIFORequestChannel::CLIENT_SIDE);
        channels.push_back(new_chan);
    }

    FIFORequestChannel chan = *(channels.back());

    if(p != -1 && t != -1 && e != 0 && filename == "") 
    {
        
        datamsg x(p, t, e);
        memcpy(buf, &x, sizeof(datamsg));
        chan.cwrite(buf, sizeof(datamsg)); // question
        double reply;
        chan.cread(&reply, sizeof(double)); //answer
        cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
    }
	// example data point request
    
	
	else if(p != -1 && t == -1 && e == 0 && filename == "") 
	{
        ofstream ofs("received/x1.csv");
        if(ofs.fail())
            EXITONERROR("Could not open x1.csv for writing");
        double ecg1;
        double ecg2;
        for(int i = 0; i<1000; i++)
        {
            datamsg m1(p,i*.004,1);
            memcpy(buf, &m1, sizeof(datamsg));
            chan.cwrite(buf, sizeof(datamsg));
            chan.cread(&ecg1, sizeof(double));

            datamsg m2(p,i*.004,2);
            memcpy(buf, &m2, sizeof(datamsg));
            chan.cwrite(buf, sizeof(datamsg));
            chan.cread(&ecg2, sizeof(double));
            ofs << i*.004 << "," << ecg1 << "," << ecg2 << "\n";
        }

        ofs.close();
    }
    
    else if(p == -1 && t == -1 && e == 0 && filename != "")
    {
        //{ sending a non-sense message, you need to change this
        filemsg fm(0, 0);
        // string fname = "1.csv";
        
        int msglen = sizeof(filemsg) + (filename.size() + 1);
        char* buf2 = new char[msglen];
        char* buf3 = new char[m];
        memcpy(buf2, &fm, sizeof(filemsg));
        strcpy(buf2 + sizeof(filemsg), filename.c_str());
        chan.cwrite(buf2, msglen);  // I want the file length;

        __int64_t filelen;
        chan.cread(&filelen, sizeof(__int64_t));

        // cout << "File size: " << filelen << " bytes" << endl;

        string outpath = "received/" + filename;
        FILE* fp = fopen(outpath.c_str(), "wb");
        if (!fp) {
            cerr << "Cannot open " << outpath << " for writing" << endl;
            exit(1);
        }

        __int64_t offset = 0;
        // auto start = chrono::high_resolution_clock::now();
        while (offset < filelen) {
            __int64_t chunk_size = min((__int64_t)m, filelen - offset);

            filemsg fm(offset, chunk_size);
            memcpy(buf2, &fm, sizeof(filemsg));
            strcpy(buf2 + sizeof(filemsg), filename.c_str());

            chan.cwrite(buf2, msglen);
            char* response = new char[chunk_size];
            int nbytes = chan.cread(response, chunk_size);

            fwrite(response, 1, nbytes, fp);

            offset += nbytes;
            delete[] response;
        }

        fclose(fp);

        // auto end = chrono::high_resolution_clock::now();
        // chrono::duration<double> elapsed = end - start;
        // cout << "check chunk_size: " << (__int64_t)msglen << endl;
        // cout << "Transfer time: " << elapsed.count() << " seconds" << endl;
        //main bottleneck is the channel buffer size

        // cout << "File received and saved to " << outpath << endl;
        delete[] buf2;
        delete[] buf3;
    }
	
    MESSAGE_TYPE n = QUIT_MSG;
    chan.cwrite(&n, sizeof(MESSAGE_TYPE));

    if(new_chan)
    {
        MESSAGE_TYPE n = QUIT_MSG;
        cont_chan.cwrite(&n, sizeof(MESSAGE_TYPE));
        delete channels.back();
    }


    int status;
    wait(&status);


    return 0;
}
