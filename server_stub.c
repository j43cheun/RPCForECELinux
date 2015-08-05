#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "ece454rpc_types.h"

#define  BUFFER_SIZE 4096

/** @struct
 
    @brief Defines a linked list element for storing registered procedures. 
*/
struct procedure_element
{
    char*                      m_procedure_name;             ///< The procedure name 
    int                        m_nparams;                    ///< The number of parameters accepted by the procedure 
    fp_type                    m_fnpointer;                  ///< The function pointer to the procedure 
    struct  procedure_element* m_sp_next_procedure_element;  ///< The pointer to the next registered procedure in the linked list 
};

/* A pointer to the head of the linked list storing registered procedures */
struct procedure_element* sp_procedure_list_head_element = NULL;

/**
 * @brief This function registers a function in server stub.
 *
 * @param procedure_name The name of the procedure to be registered.
 * @param nparams        The number of arguments accepted by the procedure.
 * @param fnpointer      The function pointer to the procedure.
 *
 * @return Returns true if the procedure was registered successfully. Returns false otherwise.
 */
bool register_procedure( const char* procedure_name, const int nparams, fp_type fnpointer )
{
    struct procedure_element* sp_procedure_element;                                               ///< Declare a pointer for a procedure element instance to be created.
    struct procedure_element* sp_procedure_list_current_element = sp_procedure_list_head_element; ///< Declare a pointer to the current element in the procedure element linked list.

    // Register the name of the procedure.
    sp_procedure_element = ( struct procedure_element* )malloc( sizeof( struct procedure_element ) );
    sp_procedure_element->m_procedure_name = ( char* )malloc( strlen( procedure_name ) + 1 );
    strcpy( sp_procedure_element->m_procedure_name, procedure_name );

    // If name was not successfully set, return false.
    if( sp_procedure_element->m_procedure_name == NULL )
    {
        free( sp_procedure_element );
        return false;
    }

    // Register the number of parameters accepted by the procedure, its function pointer, and the next procedure element.
    sp_procedure_element->m_nparams = nparams;
    sp_procedure_element->m_fnpointer = fnpointer;
    sp_procedure_element->m_sp_next_procedure_element = NULL;

    if( sp_procedure_list_head_element == NULL )
    {
        // If head is NULL, set the head to point to the new procedure element instance.
        sp_procedure_list_head_element = sp_procedure_element;
    }
    else
    {
        // Iterate to the end of the procedure element linked list.
        while( sp_procedure_list_current_element->m_sp_next_procedure_element != NULL )
        {
            sp_procedure_list_current_element = sp_procedure_list_current_element->m_sp_next_procedure_element;
        }
        
        // Set the tail of the procedure element linked list to point to the new procedure element instance.
        sp_procedure_list_current_element->m_sp_next_procedure_element = sp_procedure_element;
    }
    
    // Procedure was registered successfully.
    return true;
}

/**
 * @brief This function maps a procedure name to a procedure.
 *
 * @param  procedure_name The name of the procedure.
 *
 * @return Returns the function pointer registered to procedure_name. If no function
 *         pointer corresponding to procedure_name has been registered, return NULL.
 */
fp_type map_procedure_name_to_fnpointer( const char* procedure_name )
{
    fp_type fnpointer = NULL;                                                        ///< Declare the function pointer to be returned.
    struct procedure_element* sp_procedure_element = sp_procedure_list_head_element; ///< Declare a pointer to the current element in the procedure element linked list.

    // Iterate through procedure element linked list to find procedure element with matching procedure name.
    while( sp_procedure_element != NULL )
    {
        if( strcmp( sp_procedure_element->m_procedure_name, procedure_name ) == 0 )
        {
            // If the current procedure element has a matching procedure name, return it.
            fnpointer = sp_procedure_element->m_fnpointer;
            break;
        }
        else
        {
            // Go to the next element in the procedure element linked list.
            sp_procedure_element = sp_procedure_element->m_sp_next_procedure_element;
        }
    }
    
    // Return the function pointer.
    return fnpointer;
}

/**
 * @brief This function returns the IPv4 address corresponding to the eth0 network interface.
 *
 * @return Returns the number and dots representation of the IPv4 address for eth0
 *         as a C string.
 */
char* return_ip_addr()
{
    struct sockaddr_in* sp_sockaddr_in;    ///< Declare a pointer to a sockaddr_in instance.
    struct ifaddrs* sp_ifaddrs_head;       ///< Declare a pointer to the head of the network interfaces linked list.
    struct ifaddrs* sp_ifaddrs_current;    ///< Declare a pointer to the current element in the network interfaces linked list.
    char *addr;                            ///< Stores the IPv4 address pertaining to network interface eth0.

    // Builds a list of network interfaces and stores them starting at &sp_ifaddrs_head.
    getifaddrs( &sp_ifaddrs_head );
    
    // Traverses through every network interface.
    for( sp_ifaddrs_current = sp_ifaddrs_head; sp_ifaddrs_current; sp_ifaddrs_current = sp_ifaddrs_current->ifa_next )
    {
        // If the interface belongs to the internet family.
        if( sp_ifaddrs_current->ifa_addr->sa_family == AF_INET )
        {
            sp_sockaddr_in = ( struct sockaddr_in* )sp_ifaddrs_current->ifa_addr;
            addr = inet_ntoa( sp_sockaddr_in->sin_addr );

            // If the interface name is equal to eth0, then we found the interface we want, break.
            if( strcmp( sp_ifaddrs_current->ifa_name, "eth0" ) == 0 )
            {
                break;
            }
        }
    }
    
    // Free the block of memory allocated to the network interfaces linked list.
    freeifaddrs(sp_ifaddrs_head);

    // Return the IPv4 address corresponding to network interface eth0.
    return addr;
}

/**
 * @brief This function starts the server listening for requests for function calls
 *        to functions registered by the server stub.
 */
void launch_server()
{
    int socket_descriptor;                    ///< Stores a files descriptor pertaining to a server socket.
    int recv_size_bytes;                      ///< Stores the number of bytes received from the client.
    char* server_ip_addr;                     ///< The IPv4 address of the server.
    struct sockaddr_in s_server_sockaddr_in;  ///< Stores the server socket and port.
    struct sockaddr_in s_client_sockaddr_in;  ///< Stores the client socket and port.
    void* p_recv_buffer;                      ///< The buffer containing the remote procedure call arguments from the client.
    void* p_recv_buffer_offset;               ///< Pointer to the current value in p_recv_buffer.
    void* p_send_buffer;                      ///< The buffer containing the return value for the client.
    void* p_send_buffer_offset;               ///< Pointer to the current value in p_send_buffer.
    socklen_t addrlen;                        ///< Stores the length of s_client_sockaddr_in.
    unsigned int idx;                         ///< An index for for loops.
    arg_type* sp_arg_type_list_head;          ///< Points to the remote procedure call argument linked list.
    arg_type* sp_current_arg_type_element;    ///< Pointer to the current arg in the arg_type linked list.
    arg_type** sp_arg_type_array;             ///< An array of argtype* pointers to each arg in the arg_type linked list.
    fp_type p_fnpointer;                      ///< Call function pointer.
    return_type s_return_type;                ///< Stores the return value pertaining to the remote procedure call.

    // Establish server socket
    socket_descriptor = socket( AF_INET, SOCK_DGRAM, 0 );
    
    // If socket not established successfully, exit program.
    if( socket_descriptor < 0 )
    {
        perror( "Could not create server socket." );
        exit( 1 );
    }

    // Obtain the IP address of the current machine
    server_ip_addr = return_ip_addr();
    
    // Configure the server socket address and port number. Server can accept responses on all network interfaces.
    memset( ( char* )&s_server_sockaddr_in, 0, sizeof( s_server_sockaddr_in ) );
    s_server_sockaddr_in.sin_family = AF_INET;
    s_server_sockaddr_in.sin_addr.s_addr = htonl( INADDR_ANY );

    // Bind address and port number to UDP socket. If bind unsuccessful, exit program.
    if (mybind(socket_descriptor, &s_server_sockaddr_in) < 0)
    {
        perror("Could not bind address and port number to server socket.");
        exit( 1 );
    }
    
    // Print server IPv4 address and port number to stdout.
    printf( "%s %d\n", server_ip_addr, ntohs( s_server_sockaddr_in.sin_port ) );

    // Gets the length of s_client_sockaddr_in
    addrlen = sizeof( s_client_sockaddr_in );

    // Allocates a block of memory for incoming client RPC arguments.
    p_recv_buffer = malloc(BUFFER_SIZE);

    // Loop forever.
    while( true )
    {
        // Attempt to receive request from client.
        recv_size_bytes = recvfrom(socket_descriptor, p_recv_buffer, BUFFER_SIZE, 0, (struct sockaddr*)&s_client_sockaddr_in, &addrlen);
        
        if (recv_size_bytes <= 0)
        {
            // If could not receive request from client, set RPC return value to NULL.
            perror( "Could not receive UDP packet from client." );
            s_return_type.return_size = 0;
            s_return_type.return_val = NULL;
        }
        else if (recv_size_bytes > 0)
        {
            // Read RPC arguments from client. 
            p_recv_buffer_offset = p_recv_buffer;
            size_t procedure_name_len = *(uint32_t*)p_recv_buffer_offset;
            p_recv_buffer_offset = ( void* )( ( char* )p_recv_buffer_offset + sizeof( size_t ) );
            char* procedure_name = ( char* )p_recv_buffer_offset;
            p_recv_buffer_offset = ( void* )(( char* )p_recv_buffer_offset + procedure_name_len );
            uint32_t nparams = *(uint32_t*)p_recv_buffer_offset;
            p_recv_buffer_offset = ( void* )( ( char* )p_recv_buffer_offset + sizeof( uint32_t ) );

            sp_arg_type_list_head = NULL;
            sp_current_arg_type_element = sp_arg_type_list_head;
            sp_arg_type_array = (arg_type**)malloc(sizeof(arg_type*) * nparams);

            for (idx = 0; idx < nparams; idx++)
            {
                arg_type* s_arg_type = (arg_type *)malloc(sizeof(arg_type));
                s_arg_type->arg_size = *( size_t* )p_recv_buffer_offset;
                p_recv_buffer_offset = ( void* )( ( char* )p_recv_buffer_offset + sizeof( size_t ) );
                
                s_arg_type->arg_val = malloc(s_arg_type->arg_size);
                memcpy(s_arg_type->arg_val, p_recv_buffer_offset, s_arg_type->arg_size);
                p_recv_buffer_offset = (void*)((char*)p_recv_buffer_offset + s_arg_type->arg_size);
                s_arg_type->next = NULL;
                sp_arg_type_array[idx] = s_arg_type;
                
                if( sp_arg_type_list_head == NULL )
                {
                    sp_arg_type_list_head = s_arg_type;
                    sp_current_arg_type_element = sp_arg_type_list_head;
                }
                else
                {
                    sp_current_arg_type_element->next = s_arg_type;
                    sp_current_arg_type_element = sp_current_arg_type_element->next;
                }
            }

            // Get registered function pointer from given procedure_name.
            p_fnpointer = map_procedure_name_to_fnpointer( procedure_name );
            
            if (p_fnpointer != NULL)
            {
                // Call function pointed to by p_fnpointer if registered function exists and pass in RPC argument linked list.
                s_return_type = (*p_fnpointer)(nparams, sp_arg_type_list_head);
            }
            else
            {
                // Set RPC return value to NULL if registered function does not exist.
                s_return_type.return_size = 0;
                s_return_type.return_val = NULL;
            }

            // Deallocate memory for each arg in the RPC argument linked list.
            for(idx = 0; idx < nparams; idx++)
            {
                free(sp_arg_type_array[idx]->arg_val);
                free(sp_arg_type_array[idx]);
            }
            
            // Deallocate memory for the sp_arg_type_array.
            free(sp_arg_type_array);
        }

        // Place return value into p_send_buffer.
        p_send_buffer = malloc(sizeof(size_t) + s_return_type.return_size);
        p_send_buffer_offset = p_send_buffer;
        memcpy( p_send_buffer_offset, &( s_return_type.return_size ), sizeof( size_t ) );
        p_send_buffer_offset = ( void* )( ( char* )p_send_buffer_offset + sizeof( size_t ) );
        
        // Copy the RPC return value iff the return size is greater than 0.
        if (s_return_type.return_size > 0)
        {
            memcpy( p_send_buffer_offset, s_return_type.return_val, s_return_type.return_size );
            p_send_buffer_offset = ( void* )( ( char* )p_send_buffer_offset + s_return_type.return_size );
        }
        
        // Send the RPC return value to the client.
        if (sendto(socket_descriptor, p_send_buffer, sizeof(size_t)+s_return_type.return_size, 0, (struct sockaddr*)&s_client_sockaddr_in, addrlen) < 0)
        {
            perror("Could not return result to client.");
        }

        // Deallocate memory for the p_send_buffer.
        free(p_send_buffer);
    }
}
