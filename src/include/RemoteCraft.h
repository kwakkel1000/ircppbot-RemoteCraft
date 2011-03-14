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

	//parse functions
    void parse_privmsg();
    void ParsePrivmsg(std::vector< std::string > data);
    void StartServer();
    void StopServer();

    //post parse functions
    void Sample();

	//timer
    void timerlong();

};

#endif // RemoteCraft_H

