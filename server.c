#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Winsock2.h>
#include <WS2tcpip.h>
#include <time.h>
#include <sqlite3.h>

#define MAX_CLIENTS 5
int clientNum = 0;
int clientID = 0;

sqlite3 *db;
char *messaggeError;
int ret = 0;

typedef struct
{
    SOCKET client_sock;
    int clientID;
} ClientInfo;

ClientInfo client_DB[MAX_CLIENTS];

void clientHandler(ClientInfo *params)
{
    char buffer[1024];
    char timestamp[1024];
    int rc;
    int num = clientNum;
    char sql[1024];

    time_t t;
    time(&t);

    SOCKET client_sock = params->client_sock;
    int clientID = params->clientID;

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));

        rc = recv(client_sock, buffer, sizeof(buffer), 0);
        if (rc <= 0)
        {
            break;
        }

        int receiverID = 0;
        int receiverAuth = 0;
        char *messageContent = NULL;
        char *delimiter = strchr(buffer, ':');

        if (delimiter != NULL)
        {
            receiverID = atoi(buffer + 1);
            messageContent = delimiter + 1;

            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (client_DB[i].clientID == receiverID)
                {
                    char formattedMessage[1024];
                    memset(formattedMessage, 0, sizeof(formattedMessage));
                    sprintf(formattedMessage, "Received message from #%d: %s", clientID, messageContent);
                    send(client_DB[i].client_sock, formattedMessage, strlen(formattedMessage), 0);
                    receiverAuth = 1;
                    break;
                }
            }
        }

        if (receiverAuth == 0)
        {
            memset(buffer, 0, sizeof(buffer));
            sprintf(buffer, "[-]Client #%d not available right now\n", receiverID);
            send(client_sock, buffer, strlen(buffer), 0);
        }
        else
        {
            memset(timestamp, 0, sizeof(timestamp));
            strcpy(timestamp, ctime(&t));
            timestamp[strlen(timestamp) - 1] = '\0';

            sprintf(sql, "INSERT INTO Messages (senderID, receiverID, message, timestamp) VALUES(%d, %d, '%s', '%s');", clientID, receiverID, messageContent, timestamp);

            char *messaggeError;
            int ret = sqlite3_exec(db, sql, NULL, 0, &messaggeError);

            if (ret != SQLITE_OK)
            {
                printf("Error inserting record: %s\n", messaggeError);
            }
            else
            {
                printf("Record inserted successfully\n");
            }
        }
    }

    for (int i = 0; i < clientNum; i++)
    {
        if (client_DB[i].clientID == clientID)
        {
            client_DB[i].clientID = 0;
            client_DB[i].client_sock = INVALID_SOCKET;
            for (; i < clientNum - 1; i++)
            {
                client_DB[i] = client_DB[i + 1];
            }
            clientNum--;
            break;
        }
    }

    free(params);
    closesocket(client_sock);
    printf("----Client #%d Disconnected----\n", clientID);
}

int main()
{
    ret = sqlite3_open("client_records.db", &db);

    char *sql = "CREATE TABLE IF NOT EXISTS Messages ("
                "messageID  INTEGER PRIMARY KEY AUTOINCREMENT,"
                "senderID  INTEGER NOT NULL,"
                "receiverID  INTEGER NOT NULL,"
                "message   TEXT,"
                "timestamp TEXT NOT NULL);";

    ret = sqlite3_exec(db, sql, NULL, 0, &messaggeError);

    if (ret != SQLITE_OK)
    {
        printf("Database opening error\n");
    }
    else
    {
        printf("Table created Successfully\n");
    }

    const char *ip = "127.0.0.1";
    int port = 5566;

    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0)
    {
        printf("[-]WSAStartup failed: %d\n", result);
        return 1;
    }

    SOCKET server_sock, client_sock;
    struct sockaddr_in serverAddr, clientAddr;
    int addrSize;

    server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_sock == INVALID_SOCKET)
    {
        printf("[-]Socket creation failed\n");
        WSACleanup();
        return 1;
    }
    printf("[+]Server Socket created Successfully\n");

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ip);

    int n = bind(server_sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (n == SOCKET_ERROR)
    {
        printf("Bind Error: %d\n", WSAGetLastError());
        closesocket(server_sock);
        WSACleanup();
        return 1;
    }
    printf("[+]Bind to Port Number: %d Successfully\n", port);

    if (listen(server_sock, 5) == SOCKET_ERROR)
    {
        printf("Listen Error: %d\n", WSAGetLastError());
        closesocket(server_sock);
        WSACleanup();
        return 1;
    }
    printf("[+]Listening...\n");

    char buffer[1024];

    while (1)
    {
        addrSize = sizeof(clientAddr);
        client_sock = accept(server_sock, (struct sockaddr *)&clientAddr, &addrSize);
        if (client_sock == INVALID_SOCKET)
        {
            printf("[-]Accept Error: %d\n", WSAGetLastError());
            closesocket(server_sock);
            WSACleanup();
            return 1;
        }

        int clientAuth = 1;
        if (clientNum < MAX_CLIENTS)
        {
            memset(buffer, 0, sizeof(buffer));
            int rc = recv(client_sock, buffer, sizeof(buffer), 0);
            if (buffer[0] == '#')
            {
                clientID = atoi(buffer + 1);
            }

            ClientInfo *clientParam = (ClientInfo *)malloc(sizeof(ClientInfo));
            if (clientParam == NULL)
            {
                printf("[-]Memory allocation failed.\n");
                return 1;
            }

            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (client_DB[i].clientID == clientID)
                {
                    memset(buffer, 0, sizeof(buffer));
                    strcpy(buffer, "[-]Client ID is not unique");
                    send(client_sock, buffer, strlen(buffer), 0);
                    closesocket(client_sock);
                    clientAuth = 0;
                }
            }

            if (clientAuth == 1)
            {
                memset(buffer, 0, sizeof(buffer));
                strcpy(buffer, "[+]Client Authenticated Successfully");
                send(client_sock, buffer, strlen(buffer), 0);

                client_DB[clientNum].clientID = clientID;
                client_DB[clientNum].client_sock = client_sock;
                clientParam->client_sock = client_sock;
                clientParam->clientID = clientID;
                clientNum++;
                printf("[+]Client #%d Connected Successfully\n", clientID);

                HANDLE thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)clientHandler, (LPVOID)clientParam, 0, NULL);
                if (thread == NULL)
                {
                    printf("[-]Thread creation failed\n");
                    closesocket(client_sock);
                }
                else
                {
                    CloseHandle(thread);
                }
            }
        }
        else
        {
            printf("[-]Maximum number of clients reached. Connection rejected.\n");
            closesocket(client_sock);
        }
    }

    closesocket(server_sock);
    printf("----Connection is Closed----\n");
    WSACleanup();
    return 0;
}