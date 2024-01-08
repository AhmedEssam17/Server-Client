#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Winsock2.h>
#include <WS2tcpip.h>

int serverStatus = 1;

void recvHandler(SOCKET sock)
{
    char buffer[1024];
    int rc;

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        rc = recv(sock, buffer, sizeof(buffer), 0);
        if (rc <= 0)
        {
            printf("\n[-]Server closed the connection.\n");
            serverStatus = 0;
            break;
        }
        else{
            printf("%s", buffer);
        }
    }
}

int main(int argc, char *argv[])
{
    int clientID = atoi(argv[1]);
    if (clientID == 0)
    {
        printf("[-]Client didn't enter his ID\n");
        return 0;
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

    SOCKET sock;
    struct sockaddr_in addr;
    char buffer[1024];
    char exitCheck[1024];
    int n;

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
    {
        printf("[-]Socket creation failed\n");
        WSACleanup();
        return 1;
    }
    printf("[+]Client Socket created Successfully\n");

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        printf("[-]Connection failed: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    memset(buffer, 0, sizeof(buffer));
    buffer[0] = '#';
    strcat(buffer, argv[1]);
    send(sock, buffer, strlen(buffer), 0);

    int clientAuth = 1;
    memset(buffer, 0, sizeof(buffer));
    int auth = recv(sock, buffer, sizeof(buffer), 0);
    printf("%s\n", buffer);

    if (buffer[1] == '-')
    {
        clientAuth = 0;
    } 

    if (clientAuth == 1)
    {
        printf("[+]Client #%d Connected to the Server\n", clientID);

        HANDLE recvThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)recvHandler, (LPVOID)sock, 0, NULL);
        if (recvThread == NULL)
        {
            printf("[-]Thread creation failed\n");
            closesocket(sock);
        }
        else
        {
            CloseHandle(recvThread);
        }

        printf("\nEnter Message (Format: #receiverID:message): \n");

        while (1)
        {
            memset(buffer, 0, sizeof(buffer));
            fgets(buffer, sizeof(buffer), stdin);

            memset(exitCheck, 0, sizeof(exitCheck));
            strcpy(exitCheck, buffer);
            exitCheck[strcspn(exitCheck, "\n")] = '\0';
            if (strcmp(exitCheck, "exit") == 0 || serverStatus == 0)
            {
                break;
            }
            send(sock, buffer, strlen(buffer), 0);
        }
    }

    closesocket(sock);
    printf("----Client #%d Disconnected from Server----\n", clientID);
    WSACleanup();

    return 0;
}