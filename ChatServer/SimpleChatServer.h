
#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <regex>
#include "uWebSockets\App.h"

using std::string;
using std::vector;
using std::map;
using std::regex;

/*************************************************************************************/
//class with data of client
class simpleChatClientData
{
    public:
        //enum list with client type
        enum class ClientType
        {
            notype = 0,
            regularUser = 1,
            bot = 2,
            administrator = 3
        };

    protected:
        int clientID = 0;
        string clientName = "";
        ClientType clientType = ClientType::notype;

    public:
        simpleChatClientData()
        {
        }
        virtual ~simpleChatClientData()
        {
        }
        
        //set client ID
        void SetClientID(const int newID)
        {
            this->clientID = newID;
        }
        //get cliant ID
        int GetClientID() const
        {
            return this->clientID;
        }
        //set client name
        void SetClientName(const string& newName)
        {
            this->clientName = newName;
        }
        //get client name
        const string& GetClientName() const
        {
            return this->clientName;
        }
        //get client type
        ClientType GetClientType()
        {
            return this->clientType;
        }
};

/*************************************************************************************/
//class with bot
class simpleChatBot : public simpleChatClientData
{
    private:
        //map of questions and answers
        map<string, string> answersDataBase =
        {
            {"(hello|privet)", "Hello humanoid friend"},
            {"(how are you|how do you do|wazzap|whatup)", "Still computing PI to the trillionth digit"},
            {"what are you doing", "Answering stoopid questions"},
            {"fuck", "PLEASE DO NOT MATERITSYA"}
        };

        //upper case to lower case transform
        void lowerCase(string& text)
        {
            std::transform(text.begin(), text.end(), text.begin(), ::tolower);
        }

        //bot variables
        int answersCount = 0;
        string answerStream = "";

        //output function
        void botSay(string& output)
        {
            answerStream += output + "\n";
        }
        //overload with const string
        void botSay(const string& output)
        {
            answerStream += output + "\n";
        }

    public:
        simpleChatBot()
        {
            //set client type = bot
            this->clientType = ClientType::bot;
        }
        ~simpleChatBot()
        {
        }

        //main bot function
        const string& BotSay(string& inputMessage)
        {
            answersCount = 0;
            answerStream = "";

            string question = inputMessage;
            lowerCase(question);

            for (auto entry : answersDataBase)
            {
                regex regularExpression = regex(".*" + entry.first + ".*");
                if (regex_match(question, regularExpression))
                {
                    botSay(entry.second);
                    answersCount++;
                }
            }

            if (!answersCount)
            {
                botSay("Sorry, I don't know how to answer this :(");
            }
            if (answersCount >= 3)
            {
                botSay("Sorry for the long reply");
            }

            //delete last "\n" if exist
            if (answerStream.size() && answerStream[answerStream.length() - 1] == '\n')
            {
                answerStream.pop_back();
            }

            return answerStream;
        }
};

/*************************************************************************************/

//protocol
// [COMMAND]::[ID]::[DATA]
class SimpleChatProtocol
{
    private:
        //constants
        const string ccpSeparator = "::";

        //type of pointer to handler
        typedef string (SimpleChatProtocol::*cmdHandlerPointer)(string&, simpleChatClientData&, string&);

        //cmd #1 - private message handler
        string cmdHandler_PrivateMessage(string& cmdText, simpleChatClientData& senderData, string& msgText)
        {
            return cmdText + this->ccpSeparator + std::to_string(senderData.GetClientID()) +
                this->ccpSeparator + "[" + senderData.GetClientName() + "]:" + msgText;
        };
        //cmd #2 - set name handler
        string cmdHandler_SetName(string& cmdText, simpleChatClientData& senderData, string& msgText)
        {
            //check user name
            if (msgText.size() <= 255 && msgText.find(this->ccpSeparator) == string::npos)
            {
                //set name
                senderData.SetClientName(msgText);
            }
            return "";
        };
        //cmd #3 - get client ID
        string cmdHandler_GetClientID(string& cmdText, simpleChatClientData& senderData, string& msgText)
        {
            return cmdText + this->ccpSeparator + std::to_string(senderData.GetClientID());
        };
        
        //commands and handlers array
        struct protocolElements
        {
            string command;
            cmdHandlerPointer handlerPointer;
            bool needAnswer;
            enum
            {
                answerTargetSender,
                answerTargetReceiver
            };
            int answerTarget;
        };
        vector <protocolElements> protocolCommands =
        {
            {.command = "PRIVATE_MESSAGE",
                .handlerPointer = &SimpleChatProtocol::cmdHandler_PrivateMessage,
                .needAnswer = true,
                .answerTarget = protocolElements::answerTargetReceiver},
            {.command = "SET_NAME",
                .handlerPointer = &SimpleChatProtocol::cmdHandler_SetName,
                .needAnswer = false},
            {.command = "GET_MY_ID",
                .handlerPointer = &SimpleChatProtocol::cmdHandler_GetClientID,
                .needAnswer = true,
                .answerTarget = protocolElements::answerTargetSender},
        };

        //pointer to vector with clients
        vector <simpleChatClientData*>* chatClientsList = nullptr;

        //data of last parsing
        int lastReceiverID = 0;
        string lastMsg = "";
        
        //outgoing message
        string outMessage = "";

    public:
        SimpleChatProtocol(vector <simpleChatClientData*>* clientsListPointer)
            : chatClientsList(clientsListPointer)
        {
        }
        ~SimpleChatProtocol()
        {
        }

        //return last parsed receiver ID
        int GetLastReceiverID()
        {
            return lastReceiverID;
        }

        //return last parsed message
        string GetLastMessage()
        {
            return lastMsg;
        }

        //parse input message, save answer to "this->outMessage", return "true" if ok
        bool ParseIncomingMessage(string& senderMessage, simpleChatClientData& senderData,
            const std::function<void(string&)> messageToSenderHandler = nullptr,
            const std::function<void(string&)> messageToReceiverHandler = nullptr)
        {
            //check chatClientsList pointer not null
            if (this->chatClientsList == nullptr)
            {
                return false;
            }

            //reset outgoing message string
            this->outMessage = "";

            //find among commands
            for (auto oneProtComm : this->protocolCommands)
            {
                if (senderMessage.find(oneProtComm.command) == 0)
                {
                    //get ID from message
                    string subMsg = senderMessage.substr(oneProtComm.command.size() + this->ccpSeparator.size());
                    string subReceiverID = subMsg.substr(0, subMsg.find(this->ccpSeparator));
                    if (subReceiverID.empty())
                    {
                        subReceiverID = "0";
                    }
                    this->lastReceiverID = std::stoi(subReceiverID);
                    //get message only
                    this->lastMsg = subMsg.substr(subMsg.find(this->ccpSeparator) + this->ccpSeparator.size());

                    //check exists receiver client with this ID or not
                    auto receiverClient = std::find_if(this->chatClientsList->begin(), this->chatClientsList->end(),
                        [this](simpleChatClientData* oneClientData)
                        {
                            return oneClientData->GetClientID() == this->GetLastReceiverID();
                        });
                    if (this->GetLastReceiverID() != 0 && receiverClient == this->chatClientsList->end())
                    {
                        //client with this ID not exist
                        this->outMessage = "Client with ID = " + std::to_string(this->lastReceiverID) + " not exist!";
                        //message to sender
                        if (messageToSenderHandler != nullptr)
                        {
                            messageToSenderHandler(this->outMessage);
                        }
                        //
                        return true;
                    }

                    //send answer - to lambda??
                    auto sendAnswerFunc = [this, &oneProtComm, &senderData, &messageToSenderHandler, &messageToReceiverHandler](simpleChatClientData* receiverClient)
                    {
                        //client exists
                        //check type of client
                        if (receiverClient->GetClientType() == simpleChatClientData::ClientType::bot)
                        {
                            //chat bot answer
                            if (oneProtComm.needAnswer)
                            {
                                this->outMessage = (dynamic_cast<simpleChatBot*>(receiverClient))->BotSay(this->lastMsg);
                                //call command handler
                                if (oneProtComm.handlerPointer)
                                {
                                    this->outMessage = (this->*oneProtComm.handlerPointer)(oneProtComm.command, *receiverClient, this->outMessage);
                                }
                                if (messageToSenderHandler != nullptr)
                                {
                                    messageToSenderHandler(this->outMessage);
                                }
                                return true;
                            }
                        }
                        else
                        {
                            //for normal clients
                            //call command handler
                            if (oneProtComm.handlerPointer)
                            {
                                this->outMessage = (this->*oneProtComm.handlerPointer)(oneProtComm.command, senderData, this->lastMsg);
                            }
                        }
                        //answer message
                        if (oneProtComm.needAnswer)
                        {
                            if (oneProtComm.answerTarget == protocolElements::answerTargetSender && messageToSenderHandler != nullptr)
                            {
                                messageToSenderHandler(this->outMessage);
                            }
                            if (oneProtComm.answerTarget == protocolElements::answerTargetReceiver && messageToReceiverHandler != nullptr)
                            {
                                messageToReceiverHandler(this->outMessage);
                            }
                        }
                    };

                    //call sending of answer
                    if (this->GetLastReceiverID() != 0)
                    {
                        //to one client
                        sendAnswerFunc(*receiverClient);
                    }
                    else
                    {
                        //broadcast sending to all
                        for (auto oneReceiverClient : *(this->chatClientsList))
                        {
                            //set id
                            this->lastReceiverID = oneReceiverClient->GetClientID();
                            //send
                            sendAnswerFunc(oneReceiverClient);
                        }
                    }
                    
                    //
                    return true;
                }
            }
            return false;
        }

        //get output message
        string GetOutgoingMessage()
        {
            return this->outMessage;
        }
};

class SimpleChatServer
{
    private:
        //constants
        const string defaultClientName = "UNNAMED";
        const string clientsChannelPrefix = "user";
        const string broadcastChannelPrefix = "broadcast";
        const string defaultBotNamePrefix = "BOT ID";

        //server port
        int serverPort = 8888;
        //current clients count
        int clientsCount = 0;
        //ID of last client
        int lastClientID = 0;
        //pointers to clients
        vector <simpleChatClientData*> chatClients = {};
        //protocol object for parse messages
        SimpleChatProtocol chatProtocol = (&chatClients);


        // write debug message
        inline void dbgMsg(const string& msg)
        {
            std::cout << "[Debug message]: " << msg << std::endl;
        }

        //register new client
        bool registerNewClient(simpleChatClientData* newClientData, const std::function<void()> afterRegister = nullptr)
        {
            //check input
            if (!newClientData)
            {
                this->dbgMsg("Fail new client register: client object data is null.");
                return false;
            }

            //set new client ID and name
            newClientData->SetClientID(++this->lastClientID);
            newClientData->SetClientName(this->defaultClientName);
            //total clients count change
            this->clientsCount++;
            //add pointer to client
            this->chatClients.push_back(newClientData);
            //write debug messages
            this->dbgMsg("New client connected: ID = " + std::to_string(this->lastClientID));
            this->dbgMsg("Total clients connected: " + std::to_string(this->clientsCount));

            //run user function if not null
            if (afterRegister)
            {
                //protect user function here
                try
                {
                    afterRegister();
                }
                catch (...)
                {
                    this->dbgMsg("Fail execute additional function after client ID = " + std::to_string(this->lastClientID) + " registration.");
                }
            }

            //return
            return true;
        }

        //unregister (delete) client
        bool unregisterClient(simpleChatClientData* clientData, const std::function<void()> afterUnregister = nullptr)
        {
            //check input
            if (!clientData)
            {
                this->dbgMsg("Fail client unregister: client object data is null.");
                return false;
            }

            //get client ID
            int clientID = clientData->GetClientID();
            //total clients count change
            if (--this->clientsCount < 0)
            {
                this->clientsCount = 0;
            }
            //delete pointer to client
            auto clientIter = std::find(this->chatClients.begin(), this->chatClients.end(), clientData);
            if (clientIter != this->chatClients.end())
            {
                this->chatClients.erase(clientIter);
            }
            else
            {
                //err message
                this->dbgMsg("Error while delete client with ID = " + std::to_string(clientID) + ", can't find pointer to client data object.");
            }
            //write debug messages
            this->dbgMsg("Client ID = " + std::to_string(clientID) + " disconnected.");
            
            //run user function if not null
            if (afterUnregister)
            {
                //protect user function here
                try
                {
                    afterUnregister();
                }
                catch (...)
                {
                    this->dbgMsg("Fail execute additional function after client ID = " + std::to_string(clientID) + " unregistration.");
                }
            }

            //return
            return true;
        }

	public:
		SimpleChatServer()
		{
		}
		~SimpleChatServer()
		{
            //clear vector if not empty
            if (chatClients.size())
            {
                //find bots and delete
                for (auto clientObj : chatClients)
                {
                    if (clientObj->GetClientType() == simpleChatClientData::ClientType::bot)
                    {
                        delete clientObj;
                    }
                }
                //clear vector
                chatClients.clear();
            }
		}

        //Add bot
        int AddBot()
        {
            int newBotID = -1;

            //try create new bot
            try
            {
                simpleChatBot* newBot = new simpleChatBot;
                if (!registerNewClient(newBot))
                {
                    throw "can't create new bot object.";
                }
                //save new bot ID
                newBotID = newBot->GetClientID();
                //set bot name
                newBot->SetClientName(this->defaultBotNamePrefix + std::to_string(newBotID));
            }
            catch (const string& errMsg)
            {
                dbgMsg("Fail add new bot. Message: " + errMsg);
            }
            catch (...)
            {
                dbgMsg("Fail add new bot. Unknown error.");
            }

            return newBotID;
        }

        //Delete bot
        bool DeleteBot(int botID)
        {
            //try find bot with botID
            auto botsIter = std::find_if(this->chatClients.begin(), this->chatClients.end(), [&botID](simpleChatClientData* oneElem)
                {
                    return oneElem->GetClientID() == botID && oneElem->GetClientType() == simpleChatBot::ClientType::bot;
                }
            );
            if (botsIter == this->chatClients.end())
            {
                this->dbgMsg("Fail delete bot client, not exist client ID = " + std::to_string(botID));
                return false;
            }

            //unregister client
            if (unregisterClient(*botsIter))
            {
                //delete object
                delete *botsIter;
            }
            else
            {
                this->dbgMsg("Fail unregister and delete bot client. Operation cancelled.");
                return false;
            }

            return true;
        }

		//start server
		bool Start(int serverPort)
		{
            //config and start server
            uWS::App().ws<simpleChatClientData>("/*",
                {
                    /* Settings */
                    .idleTimeout = 3600,

                    /* Handlers */
                    .open = [this](auto* ws)
                    {
                        // on connect - set client data
                        //and run additional methods
                        this->registerNewClient(ws->getUserData(),
                            [this, ws]()
                            {
                                //subsribe
                                ws->subscribe(this->clientsChannelPrefix + std::to_string(this->lastClientID));
                                ws->subscribe(this->broadcastChannelPrefix);
                            });
                    },
                    
                    .message = [this](auto* ws, std::string_view eventText, uWS::OpCode opCode)
                    {
                        // on received message
                        //get input message string
                        string inputMessageString(eventText);
                        //run parser
                        this->chatProtocol.ParseIncomingMessage(inputMessageString, *ws->getUserData(),
                            [&ws, &opCode](string& msgStr)
                            {
                                //message to sender
                                ws->send(msgStr, opCode, true);
                            },
                            [&ws, this](string& msgStr)
                            {
                                //message to receiver
                                ws->publish(this->clientsChannelPrefix + std::to_string(this->chatProtocol.GetLastReceiverID()), msgStr);
                            });
                    },
                    
                    .close = [this](auto* ws, int code, std::string_view message)
                    {
                        // on disconnect - unregister client
                        unregisterClient(ws->getUserData());
                    }
                }).listen(serverPort, [this, &serverPort](auto* listen_socket)
                    {
                        if (listen_socket)
                        {
                            this->dbgMsg("Listening on port " + std::to_string(serverPort));
                        }
                    }).run();

			return true;
		}
};
