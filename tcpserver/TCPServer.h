#pragma once

#include "../hardware/DomoticzHardware.h"
#include "TCPClient.h"
#include "../webserver/proxyclient.h"
#include <set>
#include <vector>

namespace tcp {
namespace server {

class CTCPServer;

struct _tRemoteShareUser
{
	std::string Username;
	std::string Password;
	std::vector<unsigned long long> Devices;
};

#define RemoteMessage_id_Low 0xE2
#define RemoteMessage_id_High 0x2E


struct _tRemoteMessage
{
	uint8_t ID_Low;
	uint8_t ID_High;
	int		Original_Hardware_ID;
	//data
};

class CTCPServerIntBase
{
public:
	explicit CTCPServerIntBase(CTCPServer *pRoot);
	~CTCPServerIntBase(void);

	virtual void start() = 0;
	virtual void stop() = 0;
	virtual void stopClient(CTCPClient_ptr c) = 0;
	virtual void stopAllClients();

	void SendToAll(const int HardwareID, const unsigned long long DeviceRowID, const char *pData, size_t Length, const CTCPClientBase* pClient2Ignore);

	void SetRemoteUsers(const std::vector<_tRemoteShareUser> &users);
	std::vector<_tRemoteShareUser> GetRemoteUsers();
	unsigned int GetUserDevicesCount(const std::string &username);
protected:

	_tRemoteShareUser* FindUser(const std::string &username);

	bool HandleAuthentication(CTCPClient_ptr c, const std::string &username, const std::string &password);
	void DoDecodeMessage(const CTCPClientBase *pClient, const unsigned char *pRXCommand);

	std::vector<_tRemoteShareUser> m_users;
	CTCPServer *m_pRoot;

	std::set<CTCPClient_ptr> connections_;
	boost::mutex connectionMutex;

	friend class CTCPClient;
	friend class CSharedClient;
};

class CTCPServerInt : public CTCPServerIntBase {
public:
	CTCPServerInt(const std::string& address, const std::string& port, CTCPServer *pRoot);
	~CTCPServerInt(void);
	virtual void start();
	virtual void stop();
	/// Stop the specified connection.
	virtual void stopClient(CTCPClient_ptr c);
private:

	void handleAccept(const boost::system::error_code& error);

	/// Handle a request to stop the server.
	void handle_stop();

	/// The io_service used to perform asynchronous operations.
	boost::asio::io_service io_service_;

	boost::asio::ip::tcp::acceptor acceptor_;

	CTCPClient_ptr new_connection_;
};

#ifndef NOCLOUD
class CTCPServerProxied : public CTCPServerIntBase {
public:
	CTCPServerProxied(CTCPServer *pRoot, boost::shared_ptr<http::server::CProxyClient> proxy);
	~CTCPServerProxied(void);
	virtual void start();
	virtual void stop();
	/// Stop the specified connection.
	virtual void stopClient(CTCPClient_ptr c);

	bool OnNewConnection(const std::string &token, const std::string &username, const std::string &password);
	bool OnDisconnect(const std::string &token);
	bool OnIncomingData(const std::string &token, const unsigned char *data, size_t bytes_transferred);
	CSharedClient *FindClient(const std::string &token);
private:
	boost::shared_ptr<http::server::CProxyClient> m_pProxyClient;
};
#endif

class CTCPServer : public CDomoticzHardwareBase
{
public:
	CTCPServer();
	explicit CTCPServer(const int ID);
	~CTCPServer(void);

	bool StartServer(const std::string &address, const std::string &port);
#ifndef NOCLOUD
	bool StartServer(boost::shared_ptr<http::server::CProxyClient> proxy);
#endif
	void StopServer();
	void SendToAll(const int HardwareID, const unsigned long long DeviceRowID, const char *pData, size_t Length, const CTCPClientBase* pClient2Ignore);
	void SetRemoteUsers(const std::vector<_tRemoteShareUser> &users);
	unsigned int GetUserDevicesCount(const std::string &username);
	void stopAllClients();
	boost::signals2::signal<void(CDomoticzHardwareBase *pHardware, const unsigned char *pRXCommand, const char *defaultName, const int BatteryLevel)> sDecodeRXMessage;
	bool WriteToHardware(const char *pdata, const unsigned char length) { return true; };
	void DoDecodeMessage(const CTCPClientBase *pClient, const unsigned char *pRXCommand);
#ifndef NOCLOUD
	CTCPServerProxied *GetProxiedServer();
#endif
private:
	boost::shared_mutex m_server_mutex;
	CTCPServerInt *m_pTCPServer;
#ifndef NOCLOUD
	CTCPServerProxied *m_pProxyServer;
#endif

	boost::shared_ptr<boost::thread> m_thread;
	bool StartHardware() { return false; };
	bool StopHardware() { return false; };

	void Do_Work();
	bool b_ViaProxy;
};

} // namespace server
} // namespace tcp
