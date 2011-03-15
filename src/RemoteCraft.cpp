#include "include/RemoteCraft.h"
#include <core/Global.h>
#include <iostream>
#include <algorithm>
#include <unistd.h>
#include <cstdio>
#include <boost/algorithm/string.hpp>
#include <socket/IrcSocket.h>

extern "C" ModuleInterface* create()
{
    return new RemoteCraft;
}

extern "C" void destroy(ModuleInterface* x)
{
    delete x;
}

RemoteCraft::RemoteCraft()
{
}

RemoteCraft::~RemoteCraft()
{
    stop();
	Global::Instance().get_IrcData().DelConsumer(mpDataInterface);
    delete mpDataInterface;
}
void RemoteCraft::Init(DataInterface* pData)
{
	mpDataInterface = pData;
	mpDataInterface->Init(true, false, false, true);
    Global::Instance().get_IrcData().AddConsumer(mpDataInterface);

    timerlong();
}


void RemoteCraft::stop()
{
    run = false;
    mpDataInterface->stop();
    std::cout << "RemoteCraft::stop" << std::endl;
    privmsg_parse_thread->join();
    std::cout << "privmsg_parse_thread stopped" << std::endl;
}

void RemoteCraft::read()
{
    run = true;
    std::string chan;
    chan = Global::Instance().get_ConfigReader().GetString("remotecraftchannel");
    usleep(10000000);
    std::string send_string = "JOIN " + chan + "\r\n";
    Send(send_string);
    assert(!privmsg_parse_thread);
    privmsg_parse_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&RemoteCraft::parse_privmsg, this)));
}

void RemoteCraft::parse_privmsg()
{
    std::vector< std::string > data;
    while(run)
    {
        data = mpDataInterface->GetPrivmsgQueue();
        PRIVMSG(data, Global::Instance().get_ConfigReader().GetString("remotecrafttrigger"));
    }
}


void RemoteCraft::ParsePrivmsg(std::string nick, std::string command, std::string chan, std::vector< std::string > args, int chantrigger)
{
    std::cout << "RemoteCraft" << std::endl;
    UsersInterface& U = Global::Instance().get_Users();
    std::string auth = U.GetAuth(nick);
    if (args.size() == 0)
    {
        if (boost::iequals(command,"start"))
        {
            StartServer();
        }
        if (boost::iequals(command,"stop"))
        {
            StopServer();
        }
    }
}

void RemoteCraft::StartServer()
{
	int pid=fork();
    if (!pid)
    {
    	char *arg[] = {"bash", "start_minecraft_server", NULL};
		execvp("bash", arg);
    }
}

void RemoteCraft::StopServer()
{
	IrcSocket *parse_sock;
    try
    {
        parse_sock = new IrcSocket();
        parse_sock->Connect( "localhost", "9989" );
    }
    catch (IrcSocket::Exception& e)
    {
        std::cout << "Exception caught: " << e.Description() << " (" << e.Errornr() << ")" << std::endl;
        exit(1);
    }
    std::string irc_string;
    std::string recvdata;
    parse_sock->Recv(recvdata);
    irc_string = "PRIVMSG " + Global::Instance().get_ConfigReader().GetString("remotecraftchannel") + " :" + recvdata + "\r\n";
    Send(irc_string);
    usleep(10000000);
    parse_sock->Send("\r\n");
    parse_sock->Send("\r\n");
    parse_sock->Send(".stopwrapper\r\n");
    parse_sock->Recv(recvdata);
    irc_string = "PRIVMSG " + Global::Instance().get_ConfigReader().GetString("remotecraftchannel") + " :" + recvdata + "\r\n";
    Send(irc_string);
    usleep(10000000);

    parse_sock->Disconnect();
    delete parse_sock;
}


void RemoteCraft::timerrun()
{
    int Tijd;
    time_t t= time(0);
    Tijd = t;
    longtime++;
    if (longtime >= 30)
    {
    	std::cout << "timed" << std::endl;
        timerlong();
        longtime = 0;
    }
    for (int i = timer_sec.size() - 1; i >= 0; i--)
    {
        if (timer_sec[i] < Tijd)
        {
            std::cout << timer_command[i] << std::endl;
            timer_sec.erase(timer_sec.begin()+i);
            timer_command.erase(timer_command.begin()+i);
        }
    }
}

void RemoteCraft::timerlong()
{
    int Tijd;
    time_t t= time(0);
    Tijd = t;
    Tijd = Tijd + 100;
    for (int i = timer_long_sec.size() - 1; i >= 0; i--)
    {
        if (timer_long_sec[i] < Tijd)
        {
            std::cout << "timer_long to timer " << timer_long_command[i] << std::endl;
            timer_sec.push_back(timer_long_sec[i]);
            timer_command.push_back(timer_long_command[i]);
            timer_long_sec.erase(timer_long_sec.begin()+i);
            timer_long_command.erase(timer_long_command.begin()+i);
        }
    }
}
