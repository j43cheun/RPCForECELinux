#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "ece454rpc_types.h"

#define BUFFER_SIZE 4096

/** @struct
 
    @brief Defines a structure for a variable argument in make_remote_call().
*/
struct var_arg
{
    size_t m_arg_size; ///< The size of the variable argument in bytes
    void*  m_p_arg;    ///< A pointer to the value of the variable argument
};

/**
 * @brief Invokes a remote procedure on the server.
 *
 * @param servernameorip   The domain name or IPv4 address pertaining to the server.
 * @param serverportnumber The port number corresponding to the server process.
 * @param procedure_name   The procedure name corresponding to the procedure to be invoked on the server.
 * @param nparams          The number of variable arguments accepted by the remote procedure.
 * @param ...              A variable number of arguments of structure var_arg.
 *
 * @return The return value corresponding to the the remote procedure.
 */
return_type make_remote_call(const char* servernameorip, const int serverportnumber, const char* procedure_name, const int nparams, ... )
{
    int socket_descriptor;                                     ///< Stores the file descriptor pertaining to the established socket.
    unsigned int idx;                                          ///< An index for for loops.
    unsigned int var_arg_list_size = 0;                        ///< Stores the number of variable arguments.     
    struct sockaddr_in sp_server_sockaddr_in;                  ///< Stores the server socket address and port.
    struct sockaddr_in sp_client_sockaddr_in;                  ///< Stores the client socket address and port.
    struct var_arg* sp_var_arg_array;                          ///< An array that stores the variable arguments for the remote procedure call.
    struct var_arg s_var_arg;                                  ///< The current variable argument in the variable argument array.
    socklen_t addrlen = sizeof(sp_server_sockaddr_in);         ///< Stores the length of sp_server_sockaddr_in.
    return_type s_return_type;                                 ///< Stores the return value pertaining to the remote procedure call.
    size_t procedure_name_length = strlen(procedure_name) + 1; ///< Stores the number of characters in procedure_name including terminating null character.
    va_list var_arg_list;                                      ///< Stores a list of unconstrained arguments.
    void* p_send_buffer;                                       ///< Buffer containing remote procedure call arguments to be sent to the server.
    void* p_send_buffer_offset;                                ///< Pointer to the current value in p_send_buffer. 
    void* p_recv_buffer;                                       ///< Buffer containing return value to be received from the server.
    void* p_recv_buffer_offset;                                ///< Pointer to the current value in p_recv_buffer.
    size_t p_send_buffer_size;                                 ///< Defines the size of the buffer in bytes to be sent to the server.
    
    // Establish a UDP socket on the client.
    socket_descriptor = socket(AF_INET, SOCK_DGRAM, 0);

    // Check if socket was established successfully. If not, return NULL.
    if( socket_descriptor < 0 )
    {
        perror( "Could not create socket." );
        s_return_type.return_size = 0;
        s_return_type.return_val = NULL;
        return s_return_type;
    }

    // Configure the client socket address and port number. Client can accept responses on all network interfaces.
    memset( ( char* )&sp_client_sockaddr_in, 0, sizeof( sp_client_sockaddr_in ) );
    sp_client_sockaddr_in.sin_family = AF_INET;
    sp_client_sockaddr_in.sin_addr.s_addr = htonl( INADDR_ANY );
    sp_client_sockaddr_in.sin_port = htons( 0 );

    // Bind address and port number to UDP socket. If bind unsuccessful, return NULL.
    if( bind( socket_descriptor, ( struct sockaddr* )&sp_client_sockaddr_in, sizeof( sp_client_sockaddr_in ) ) < 0 )
    {
        perror( "Could not bind client address to socket." );
        s_return_type.return_size = 0;
        s_return_type.return_val = NULL;
        return s_return_type;
    }
    
    // Configure the server port number.
    memset( ( void* )&sp_server_sockaddr_in, 0, sizeof( sp_server_sockaddr_in ) );
    sp_server_sockaddr_in.sin_family = AF_INET;
    sp_server_sockaddr_in.sin_port = htons( serverportnumber );

    // Configure the server IPv4 address. If not able to set server IPv4 address, return NULL.
    if( inet_aton( servernameorip, &sp_server_sockaddr_in.sin_addr ) == 0 )
    {
        perror( "inet_aton() failed!" );
        s_return_type.return_size = 0;
        s_return_type.return_val = NULL;
        return s_return_type;
    }
    
    // Allocate memory for the variable argument array.
    sp_var_arg_array = ( struct var_arg* )malloc( sizeof( struct var_arg ) * nparams );

    // Start reading in variable arguments.
    va_start( var_arg_list, nparams );
    
    // Iterate over all variable arguments and store in variable argument array.
    for( idx = 0; idx < nparams; idx++ )
    {
        s_var_arg = va_arg( var_arg_list, struct var_arg );
        var_arg_list_size += s_var_arg.m_arg_size;
        sp_var_arg_array[idx] = s_var_arg;
    }
    
    // Stop reading in variable arguments.
    va_end( var_arg_list );

    // Takes all the values for the remote procedure call and places them into p_send_buffer.
    p_send_buffer_size = sizeof( size_t ) + strlen( procedure_name ) + 1  + sizeof( uint32_t ) + var_arg_list_size + ( sizeof( size_t ) * nparams );
    p_send_buffer = malloc( p_send_buffer_size );
    p_send_buffer_offset = p_send_buffer;
    memcpy( p_send_buffer_offset, &procedure_name_length, sizeof( size_t ) );
    p_send_buffer_offset = ( void* )( ( char* )p_send_buffer_offset + sizeof( size_t ) );
    strcpy( ( char* )p_send_buffer_offset, procedure_name );
    p_send_buffer_offset = ( void* )( ( char* )p_send_buffer_offset + procedure_name_length );
    memcpy( p_send_buffer_offset, &nparams, sizeof( uint32_t ) );
    p_send_buffer_offset = ( void* )( ( char* )p_send_buffer_offset + sizeof( uint32_t ) );

    for( idx = 0; idx < nparams; idx++ )
    {
        memcpy( p_send_buffer_offset, &(sp_var_arg_array[idx].m_arg_size), sizeof( size_t ) );
        p_send_buffer_offset = ( void* )( ( char* )p_send_buffer_offset + sizeof( size_t ) );
        memcpy( p_send_buffer_offset, sp_var_arg_array[idx].m_p_arg, sp_var_arg_array[idx].m_arg_size );
        p_send_buffer_offset = ( void* )( ( char* )p_send_buffer_offset + sp_var_arg_array[idx].m_arg_size );
    }

    // Send values in p_send_buffer to the server. If send fails, return NULL.
    if( sendto( socket_descriptor, p_send_buffer, p_send_buffer_size, 0, ( struct sockaddr* )&sp_server_sockaddr_in, addrlen) < 0 )
    {
        perror( "Failed to send packet to server." );
        s_return_type.return_size = 0;
        s_return_type.return_val = NULL;
        return s_return_type;
    }

    // Allocate memory on the heap for p_recv_buffer.
    p_recv_buffer = malloc( BUFFER_SIZE );

    // Attempt to receive response from server.
    if( recvfrom( socket_descriptor, p_recv_buffer, BUFFER_SIZE, 0, ( struct sockaddr* )&sp_client_sockaddr_in, &addrlen ) >= 0 )
    {
        // If response received successfully, read in return value from server and return it to calling function.
        p_recv_buffer_offset = p_recv_buffer;
        s_return_type.return_size = *( size_t* )p_recv_buffer_offset;
        p_recv_buffer_offset = ( void* )( ( char* )p_recv_buffer_offset + sizeof( size_t ) );

        if( s_return_type.return_size > 0 )
        {
            s_return_type.return_val = malloc(s_return_type.return_size);
            memcpy( s_return_type.return_val, p_recv_buffer_offset, s_return_type.return_size );
        }
        else
        {
            s_return_type.return_size = 0;
            s_return_type.return_val = NULL;
        } 
    }
    else
    {
        // If response not received successfully, return a NULL value to the calling function.
        perror("Could not receive response from server.");
        s_return_type.return_size = 0;
        s_return_type.return_val = NULL;
    }

    // Close the client socket.
    close( socket_descriptor );

    // Deallocate memory for sp_var_arg_array.
    free( sp_var_arg_array );
    
    // Deallocate memory for p_send_buffer.
    free( p_send_buffer );
    
    // Deallocate memory for p_recv_buffer.
    free( p_recv_buffer );
    
    // Return RPC return value to calling function.
    return s_return_type;
}
