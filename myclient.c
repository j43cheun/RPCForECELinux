#include <stdio.h>
#include <stdlib.h>
#include "ece454rpc_types.h"

// OLD PORT: 5673 Replace 56211 below with this in FINAL

int main( int argc, char* argv[] )
{
    int a = -10, b = 20;
    int v = 1, w = 2, x = 3, y = 4, z = 5;
    int waka1 = 18, waka2 = 19;

    if (argc != 3)
    {
        perror( "The number of arguments are invalid. Exitting...\n" );
        exit( 1 );
    }

    char* serveraddr = argv[1];
    int serverport = atoi( argv[2] );

    // DEBUG
    printf( "SERVERADDR: %s SERVERPORT: %d\n", serveraddr, serverport );
    
    return_type ans = make_remote_call( serveraddr,
	                                    serverport,
                                        "multtwo", 2,
	                                    sizeof( int ), ( void * )( &a ),
	                                    sizeof( int ), ( void * )( &b ) );
    
    
    return_type ans2 = make_remote_call( serveraddr,
                                         serverport,
                                         "multfive", 5,
                                         sizeof( int ), ( void * )( &v ),
                                         sizeof( int ), ( void * )( &w ),
                                         sizeof( int ), ( void * )( &x ),
                                         sizeof( int ), ( void * )( &y ),
                                         sizeof( int ), ( void * )( &z ));

     return_type ans3 = make_remote_call( serveraddr,
                                            serverport,
                                        "addtwo", 2,
                                            sizeof( int ), ( void * )( &waka1 ),
                                            sizeof( int ), ( void * )( &waka2 ) );

    

    int i = *( int * )( ans.return_val );
    int derp = *( int *)( ans2.return_val );
    int wakaderp = *( int *)( ans3.return_val );
    printf( "client, got result: %d %d %d\n", i, derp, wakaderp );

    return 0;
}
