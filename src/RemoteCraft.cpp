#include "include/RemoteCraft.h"
#include <core/Global.h>
#include <iostream>
#include <algorithm>
#include <unistd.h>
#include <cstdio>
#include <sstream>
#include <cstring>
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
    int Tijd;
    time_t t= time(0);
    Tijd = t;
	int backup_time = convertString(Global::Instance().get_ConfigReader().GetString("backuptime"));
    timer_long_sec.push_back(Tijd + backup_time);
    timer_long_command.push_back("backup");

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
    std::string pass;
    chan = Global::Instance().get_ConfigReader().GetString("remotecraftchannel");
    pass = Global::Instance().get_ConfigReader().GetString("remotecraftchannelpass");
    usleep(10000000);
    std::string send_string = "JOIN " + chan + " " + pass + "\r\n";
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
            StartServer(nick, false);
        }
        if (boost::iequals(command,"stop"))
        {
            StopServer(nick);
        }
        if (boost::iequals(command,"save"))
        {
            SaveServer(nick);
        }
        if (boost::iequals(command,"kill"))
        {
            KillServer(nick);
        }
    }
    if (args.size() == 1)
    {
        if (boost::iequals(command,"start"))
        {
        	if (boost::iequals(args[0], "force"))
        	{
				StartServer(nick, true);
        	}
        }
    }
}

void RemoteCraft::StartServer(std::string nick, bool force)
{
    UsersInterface& U = Global::Instance().get_Users();
	int oaccess = U.GetOaccess(nick);
	if (oaccess >= 5)
	{
		bool start_up = false;
		if (force)
		{
			std::string irc_string = "";
			irc_string = "PRIVMSG " + Global::Instance().get_ConfigReader().GetString("remotecraftchannel") + " :forced starting up\r\n";
			Send(irc_string);
			start_up = true;
		}
		else
		{
			IrcSocket *client_socket;
			try
			{
				client_socket = new IrcSocket();
				client_socket->Connect( "localhost", Global::Instance().get_ConfigReader().GetString("remotecrafttcpport") );
				std::string irc_string = "";
				std::string recvdata = "";
				std::string name = Global::Instance().get_ConfigReader().GetString("remotecraftname") + "\r\n";
				std::string pass = Global::Instance().get_ConfigReader().GetString("remotecraftpass") + "\r\n";
				usleep(2000000);
				client_socket->Send(name);
				client_socket->Send(pass);
				usleep(2000000);
				client_socket->Disconnect();
				delete client_socket;
				irc_string = "PRIVMSG " + Global::Instance().get_ConfigReader().GetString("remotecraftchannel") + " :server still running\r\n";
				Send(irc_string);
			}
			catch (IrcSocket::Exception& e)
			{
				std::string irc_string = "";
				irc_string = "PRIVMSG " + Global::Instance().get_ConfigReader().GetString("remotecraftchannel") + " :server down. starting up\r\n";
				Send(irc_string);
				start_up = true;
			}
		}
		if (start_up)
		{
			int pid=fork();
			if (!pid)
			{
				char* program;
				std::string exe = Global::Instance().get_ConfigReader().GetString("remotecraftstartexecutable");
				program = new char [exe.size()+1];
				strcpy (program, exe.c_str());
				char *arg[] = {"bash", program, NULL};
				execvp("bash", arg);
			}
			usleep(2000000);
			std::string irc_string = "PRIVMSG " + Global::Instance().get_ConfigReader().GetString("remotecraftchannel") + " :the server is probably started\r\n";
			Send(irc_string);
		}
		else
		{
			usleep(2000000);
			std::string irc_string = "PRIVMSG " + Global::Instance().get_ConfigReader().GetString("remotecraftchannel") + " :the server is not started\r\n";
			Send(irc_string);
		}
	}
}

void RemoteCraft::StopServer(std::string nick)
{
    UsersInterface& U = Global::Instance().get_Users();
	int oaccess = U.GetOaccess(nick);
	if (oaccess >= 5)
	{
		IrcSocket *client_socket;
		try
		{
			client_socket = new IrcSocket();
			client_socket->Connect( "localhost", Global::Instance().get_ConfigReader().GetString("remotecrafttcpport") );
			std::string irc_string = "";
			std::string recvdata = "";
			std::string name = Global::Instance().get_ConfigReader().GetString("remotecraftname") + "\r\n";
			std::string pass = Global::Instance().get_ConfigReader().GetString("remotecraftpass") + "\r\n";
			usleep(2000000);
			client_socket->Send(name);
			client_socket->Send(pass);
			usleep(2000000);
			client_socket->Send("save-all\r\n");
			usleep(2000000);
			irc_string = "PRIVMSG " + Global::Instance().get_ConfigReader().GetString("remotecraftchannel") + " :server saving\r\n";
			Send(irc_string);
			client_socket->Send("save-off\r\n");
			usleep(2000000);
			client_socket->Send("save-on\r\n");
			usleep(10000000);
			irc_string = "PRIVMSG " + Global::Instance().get_ConfigReader().GetString("remotecraftchannel") + " :server save done\r\n";
			Send(irc_string);
			client_socket->Send(".stopwrapper\r\n");
			usleep(2000000);
			irc_string = "PRIVMSG " + Global::Instance().get_ConfigReader().GetString("remotecraftchannel") + " :server stopped wait 10seconds before starting it again\r\n";
			Send(irc_string);
			client_socket->Disconnect();
			delete client_socket;
			irc_string = "PRIVMSG " + Global::Instance().get_ConfigReader().GetString("remotecraftchannel") + " :socked closed\r\n";
			Send(irc_string);
		}
		catch (IrcSocket::Exception& e)
		{
			std::string irc_string = "";
			std::cout << "Exception caught: " << e.Description() << " (" << e.Errornr() << ")" << std::endl;
			std::stringstream ss;//create a stringstream
			ss << e.Errornr();//add number to the stream
			irc_string = "PRIVMSG " + Global::Instance().get_ConfigReader().GetString("remotecraftchannel") + " :" +  e.Description() + " (" + ss.str() + ")\r\n";
			Send(irc_string);
			irc_string = "PRIVMSG " + Global::Instance().get_ConfigReader().GetString("remotecraftchannel") + " :server couldnt be stopped\r\n";
			Send(irc_string);
		}
	}
}

void RemoteCraft::SaveServer(std::string nick)
{
    UsersInterface& U = Global::Instance().get_Users();
	int oaccess = U.GetOaccess(nick);
	if (oaccess >= 5)
	{
		save();
	}
}

void RemoteCraft::KillServer(std::string nick)
{
    UsersInterface& U = Global::Instance().get_Users();
	int oaccess = U.GetOaccess(nick);
	if (oaccess >= 5)
	{
		int pid=fork();
		if (!pid)
		{
			char* program;
			std::string exe = Global::Instance().get_ConfigReader().GetString("remotecraftkill");
			program = new char [exe.size()+1];
			strcpy (program, exe.c_str());
			char *arg[] = {"bash", program, NULL};
			execvp("bash", arg);
		}
		usleep(2000000);
		std::string irc_string = "PRIVMSG " + Global::Instance().get_ConfigReader().GetString("remotecraftchannel") + " :the server is hardly killed\r\n";
		Send(irc_string);
	}
}

void RemoteCraft::save()
{
	IrcSocket *client_socket;
	try
	{
		client_socket = new IrcSocket();
		client_socket->Connect( "localhost", Global::Instance().get_ConfigReader().GetString("remotecrafttcpport") );
		std::string irc_string = "";
		std::string recvdata = "";
		std::string name = Global::Instance().get_ConfigReader().GetString("remotecraftname") + "\r\n";
		std::string pass = Global::Instance().get_ConfigReader().GetString("remotecraftpass") + "\r\n";
		usleep(2000000);
		client_socket->Send(name);
		client_socket->Send(pass);
		usleep(2000000);
		client_socket->Send("save-all\r\n");
		usleep(2000000);
		irc_string = "PRIVMSG " + Global::Instance().get_ConfigReader().GetString("remotecraftchannel") + " :server saving\r\n";
		Send(irc_string);
		client_socket->Send("save-off\r\n");
		usleep(2000000);
		client_socket->Send("save-on\r\n");
		usleep(10000000);
		irc_string = "PRIVMSG " + Global::Instance().get_ConfigReader().GetString("remotecraftchannel") + " :server save done\r\n";
		Send(irc_string);
		client_socket->Disconnect();
		delete client_socket;
		irc_string = "PRIVMSG " + Global::Instance().get_ConfigReader().GetString("remotecraftchannel") + " :socked closed\r\n";
		Send(irc_string);
	}
	catch (IrcSocket::Exception& e)
	{
		std::string irc_string = "";
		std::cout << "Exception caught: " << e.Description() << " (" << e.Errornr() << ")" << std::endl;
		std::stringstream ss;//create a stringstream
		ss << e.Errornr();//add number to the stream
		irc_string = "PRIVMSG " + Global::Instance().get_ConfigReader().GetString("remotecraftchannel") + " :" +  e.Description() + " (" + ss.str() + ")\r\n";
		Send(irc_string);
		irc_string = "PRIVMSG " + Global::Instance().get_ConfigReader().GetString("remotecraftchannel") + " :server couldnt be saved\r\n";
		Send(irc_string);
	}
}

void RemoteCraft::timerrun()
{
    int Tijd;
    time_t t= time(0);
    Tijd = t;
    longtime++;
    if (longtime >= 30)
    {
        timerlong();
        longtime = 0;
    }
    for (int i = timer_sec.size() - 1; i >= 0; i--)
    {
        if (timer_sec[i] < Tijd)
        {
			int Tijd;
			time_t t= time(0);
			Tijd = t;
			int backup_time = convertString(Global::Instance().get_ConfigReader().GetString("backuptime"));
            std::cout << timer_command[i] << std::endl;
            timer_sec.erase(timer_sec.begin()+i);
            timer_command.erase(timer_command.begin()+i);

            save();

			timer_long_sec.push_back(Tijd + backup_time);
			timer_long_command.push_back("backup");
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
