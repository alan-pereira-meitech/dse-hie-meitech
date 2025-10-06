#include "pch.h"
#include "JetBusConnection.h"

#using <System.dll>
#using <System.Net.Http.dll>

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Threading;
using namespace Hbm::Automation::Api;
using namespace Hbm::Automation::Api::Weighing::WTX::Jet;
using namespace Hbm::Devices::Jet;
using namespace Newtonsoft::Json::Linq;

namespace
{
    const String^ STD_USER = "Administrator";
    const String^ STD_PASSWORD = "wtx";
}

JetBusConnection::JetBusConnection(String^ ipAddress)
{
    Initialize(ipAddress, STD_USER, STD_PASSWORD);
}

JetBusConnection::JetBusConnection(String^ ipAddress, String^ user, String^ password)
{
    Initialize(ipAddress, user, password);
}

JetBusConnection::~JetBusConnection()
{
    if (!_disposed)
    {
        Disconnect();
        _disposed = true;
    }
}

JetBusConnection::!JetBusConnection()
{
    Disconnect();
}

void JetBusConnection::Initialize(String^ ipAddress, String^ user, String^ password)
{
    if (String::IsNullOrWhiteSpace(ipAddress))
    {
        throw gcnew ArgumentException("IP address must be provided", "ipAddress");
    }

    _ipAddress = ipAddress;
    _user = String::IsNullOrWhiteSpace(user) ? STD_USER : user;
    _password = String::IsNullOrWhiteSpace(password) ? STD_PASSWORD : password;
    _successEvent = gcnew AutoResetEvent(false);
    _localException = nullptr;
    _allData = gcnew Dictionary<String^, String^>();
    _isConnected = false;
    _timeoutMs = 20000;

    String^ uri = String::Format("wss://{0}:443/jet/canopen", ipAddress);
    IJetConnection^ jetConnection = gcnew WebSocketJetConnection(uri, gcnew RemoteCertificateValidationCallback(WebSocketJetConnection::SkipCertificateValidation));
    _peer = gcnew JetPeer(jetConnection);
    _peer->DataReceived += gcnew EventHandler<JetDataReceivedEventArgs^>(this, &JetBusConnection::PeerOnData);
    _peer->Disconnected += gcnew EventHandler(this, &JetBusConnection::PeerOnDisconnected);
}

String^ JetBusConnection::IpAddress::get()
{
    return _ipAddress;
}

void JetBusConnection::IpAddress::set(String^ value)
{
    _ipAddress = value;
}

Hbm::Automation::Api::ConnectionType JetBusConnection::ConnectionType::get()
{
    return Hbm::Automation::Api::ConnectionType::Jetbus;
}

bool JetBusConnection::IsConnected::get()
{
    return _isConnected;
}

Dictionary<String^, String^>^ JetBusConnection::AllData::get()
{
    return _allData;
}

void JetBusConnection::Connect(int timeoutMs)
{
    _timeoutMs = timeoutMs;
    ConnectPeer(_user, _password, timeoutMs);
    WaitOne(3);
}

void JetBusConnection::Disconnect()
{
    if (_peer != nullptr)
    {
        try
        {
            _peer->Disconnect();
        }
        catch (JetPeerException^)
        {
        }
    }

    _isConnected = false;
}

void JetBusConnection::ConnectPeer(String^ user, String^ password, int timeoutMs)
{
    _successEvent->Reset();
    _localException = nullptr;

    JetCredentials^ credentials = gcnew JetCredentials(user, password);
    _peer->Connect(credentials, timeoutMs);

    _isConnected = true;
}

String^ JetBusConnection::Read(Object^ command)
{
    return ReadFromBuffer(command);
}

String^ JetBusConnection::ReadFromBuffer(Object^ command)
{
    JetBusCommand^ jetCommand = dynamic_cast<JetBusCommand^>(command);
    if (jetCommand == nullptr)
    {
        throw gcnew ArgumentException("Command must be a JetBusCommand", "command");
    }

    String^ raw;
    if (!_allData->TryGetValue(jetCommand->Path, raw))
    {
        return "0";
    }

    return jetCommand->ToString(raw);
}

String^ JetBusConnection::ReadFromDevice(Object^ command)
{
    JetBusCommand^ jetCommand = dynamic_cast<JetBusCommand^>(command);
    if (jetCommand == nullptr)
    {
        throw gcnew ArgumentException("Command must be a JetBusCommand", "command");
    }

    JetBusNode^ node = _peer->Device->GetNode(jetCommand->Path);
    JToken^ token = node->Read();
    String^ value = token->ToString();
    _allData[jetCommand->Path] = value;
    return jetCommand->ToString(value);
}

int JetBusConnection::ReadIntegerFromBuffer(Object^ command)
{
    JetBusCommand^ jetCommand = dynamic_cast<JetBusCommand^>(command);
    if (jetCommand == nullptr)
    {
        throw gcnew ArgumentException("Command must be a JetBusCommand", "command");
    }

    String^ raw;
    if (!_allData->TryGetValue(jetCommand->Path, raw))
    {
        return 0;
    }

    return jetCommand->ToSValue(raw);
}

bool JetBusConnection::WriteInteger(Object^ command, int value)
{
    return Write(command, value.ToString());
}

bool JetBusConnection::Write(Object^ command, String^ value)
{
    JetBusCommand^ jetCommand = dynamic_cast<JetBusCommand^>(command);
    if (jetCommand == nullptr)
    {
        throw gcnew ArgumentException("Command must be a JetBusCommand", "command");
    }

    JValue^ payload = gcnew JValue(value);
    SetData(jetCommand->Path, payload);
    WaitOne(1);
    return true;
}

void JetBusConnection::PeerOnData(System::Object^ sender, JetDataReceivedEventArgs^ e)
{
    if (e == nullptr || e->NodeValues == nullptr)
    {
        return;
    }

    for each(KeyValuePair<String^, String^> pair in e->NodeValues)
    {
        _allData[pair.Key] = pair.Value;
    }

    if (UpdateData != nullptr)
    {
        UpdateData(this, EventArgs::Empty);
    }
}

void JetBusConnection::PeerOnDisconnected(System::Object^ sender, EventArgs^ e)
{
    _isConnected = false;
}

void JetBusConnection::WaitOne(int seconds)
{
    if (!_successEvent->WaitOne(TimeSpan::FromSeconds(seconds)))
    {
        if (_localException != nullptr)
        {
            throw _localException;
        }
    }
}

void JetBusConnection::SetData(String^ path, JValue^ value)
{
    JetBusNode^ node = _peer->Device->GetNode(path);
    node->Write(value);
}
