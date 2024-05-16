#include "../enclosure_controller.h"
#include "../secrets.h"

WiFiClientSecure client;

const int VERBOSITY = SSH_LOG_PROTOCOL;

void EnclosureController::_ssh_init()
{
    // WiFiClientSecure client;
    // client.setInsecure();

    // if (!client.connect(SSH_HOST, SSH_PORT))
    // {
    //     log_w("Connection failed");
    //     return;
    // }

    // Initialize SSH session
    ssh_session _ssh_session = ssh_new();
    if (_ssh_session == NULL) {
        log_w("Error creating session");
        return;
    }

    ssh_options_set(_ssh_session, SSH_OPTIONS_HOST, SSH_HOST);
    ssh_options_set(_ssh_session, SSH_OPTIONS_USER, SSH_USER);
    //ssh_options_set(_ssh_session, SSH_OPTIONS_PORT, &sshPort);
    ssh_options_set(_ssh_session, SSH_OPTIONS_LOG_VERBOSITY, &VERBOSITY);

    int rc = ssh_connect(_ssh_session);
    if (rc != SSH_OK)
    {
        log_w("Error connecting: %s", ssh_get_error(_ssh_session));
        ssh_free(_ssh_session);
        return;
    }

    // rc = ssh_userauth_password(_ssh_session, NULL, SSH_PASS);
    // if (rc != SSH_AUTH_SUCCESS)
    // {
    //     log_w("Authentication error: %s", ssh_get_error(_ssh_session));
    //     ssh_disconnect(_ssh_session);
    //     ssh_free(_ssh_session);
    //     return;
    // }

    // ssh_channel channel = ssh_channel_new(_ssh_session);
    // if (channel == NULL) {
    //     log_w("Error creating channel: %s", ssh_get_error(_ssh_session));
    //     ssh_disconnect(_ssh_session);
    //     ssh_free(_ssh_session);
    //     return;
    // }
    
    // rc = ssh_channel_open_session(channel);
    // if (rc != SSH_OK)
    // {
    //     log_w("Error opening channel: %s", ssh_get_error(_ssh_session));
    //     ssh_channel_free(channel);
    //     return;
    // }

    // rc = ssh_channel_request_exec(channel, "ls");
    // if (rc != SSH_OK)
    // {
    //     log_w("Error executing command: %s", ssh_get_error(_ssh_session));
    //     ssh_channel_close(channel);
    //     ssh_channel_free(channel);
    //     return;
    // }

    // // Receive the response (optional)
    // char buffer[256];
    // int nbytes;
    // while ((nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0)) > 0)
    // {
    //     Serial.write(buffer, nbytes);
    // }

    _ssh_inited = true;
    // ssh_channel_send_eof(channel);
    // ssh_channel_close(channel);
    // ssh_channel_free(channel);
    // ssh_disconnect(session);
    // ssh_free(session);
}

// void FactoryTest::_ssh_cmd(const char* cmd)
// {
//     if (!_ssh_inited)
//     {
//         log_w("SSH not initialized");
//         return;
//     }

//     ssh_channel channel = ssh_channel_new(session);
//     if (channel == NULL) {
//         log_w("Error creating channel: %s", ssh_get_error(session));
//         ssh_disconnect(session);
//         ssh_free(session);
//         return;
//     }
    
//     int rc = ssh_channel_open_session(channel);
//     if (rc != SSH_OK)
//     {
//         log_w("Error opening channel: %s", ssh_get_error(session));
//         ssh_channel_free(channel);
//         return;
//     }

//     rc = ssh_channel_request_exec(channel, cmd);
//     if (rc != SSH_OK)
//     {
//         log_w("Error executing command: %s", ssh_get_error(session));
//         ssh_channel_close(channel);
//         ssh_channel_free(channel);
//         return;
//     }

//     // Receive the response (optional)
//     char buffer[256];
//     int nbytes;
//     while ((nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0)) > 0)
//     {
//         Serial.write(buffer, nbytes);
//     }

//     ssh_channel_send_eof(channel);
//     ssh_channel_close(channel);
//     ssh_channel_free(channel);
// }