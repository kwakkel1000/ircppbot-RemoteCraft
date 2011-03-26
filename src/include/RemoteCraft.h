#ifndef RemoteCraft_H
#define RemoteCraft_H
#include <core/ModuleBase.h>
#include <interfaces/DataInterface.h>
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/thread/thread.hpp>

class DataInterface;
class RemoteCraft : public ModuleBase
{
public:
    RemoteCraft();
    ~RemoteCraft();
    void read();
    void stop();
    void Init(DataInterface* pData);
    void timerrun();

private:
	//vars

    bool run;

	//classes
    DataInterface* mpDataInterface;

	//timer vars
    std::vector<int> timer_sec;
    std::vector< std::string > timer_command;
    std::vector<int> timer_long_sec;
    std::vector< std::string > timer_long_command;
    int longtime;

	//thread vars
    boost::shared_ptr<boost::thread> privmsg_parse_thread;

	//functions
	void runConsoleCommand(std::vector< std::string > args);
	std::string GetHashKey(std::string command);
    void sha256(char* input, char output[65]);
    //void sha512(char* input, char output[129]);

	//parse functions
    void parse_privmsg();
    void ParsePrivmsg(std::string nick, std::string command, std::string chan, std::vector< std::string > args, int chantrigger);
    void StartServer(std::string nick, bool force);
    void StopServer(std::string nick);
    void SaveServer(std::string nick);
    void KillServer(std::string nick);

    //post parse functions
    void save();

	//timer
    void timerlong();

};

#endif // RemoteCraft_H

