#include <stdio.h>
#include "ece454rpc_types.h"

int ret_int;
return_type r;

return_type add( const int nparams, arg_type* a )
{
    printf("Entered the add function.\n");
    
    if( nparams != 2 )
    {
        /* Error! */
        r.return_val = NULL;
        r.return_size = 0;
        return r;
    }

    if( a->arg_size != sizeof( int ) || a->next->arg_size != sizeof( int ) )
    {
        /* Error! */
        r.return_val = NULL;
        r.return_size = 0;
        return r;
    }

    int i = *( int * )( a->arg_val );
    int j = *( int * )( a->next->arg_val );

    printf("i = %d, j = %d\n", i, j);

    ret_int = i + j;
    r.return_val = ( void * )( &ret_int );
    r.return_size = sizeof( int );

    return r;
}

return_type multiply(const int nparams, arg_type* a)
{
    printf("Entered the multiply function.\n");

    if (nparams != 2)
    {
        /* Error! */
        r.return_val = NULL;
        r.return_size = 0;
        return r;
    }

    if (a->arg_size != sizeof(int) || a->next->arg_size != sizeof(int))
    {
        /* Error! */
        r.return_val = NULL;
        r.return_size = 0;
        return r;
    }

    int i = *(int *)(a->arg_val);
    int j = *(int *)(a->next->arg_val);

    printf("i = %d, j = %d\n", i, j);

    ret_int = i * j;
    r.return_val = (void *)(&ret_int);
    r.return_size = sizeof(int);

    return r;
}

return_type multiplyFive(const int nparams, arg_type* a)
{
    printf("Entered the multiplyFive function.\n");

    if (nparams != 5)
    {
        /* Error! */
        r.return_val = NULL;
        r.return_size = 0;
        return r;
    }

    if (a->arg_size != sizeof(int) || a->next->arg_size != sizeof(int))
    {
        /* Error! */
        r.return_val = NULL;
        r.return_size = 0;
        return r;
    }

    int i = *(int *)(a->arg_val);
    int j = *(int *)(a->next->arg_val);
    int k = *(int *)(a->next->next->arg_val);
    int l = *(int *)(a->next->next->next->arg_val);
    int m = *(int *)(a->next->next->next->next->arg_val);

    printf("i=%d j=%d k=%d l=%d m=%d\n", i, j, k, l, m);

    ret_int = i * j * k * l * m;
    r.return_val = (void *)(&ret_int);
    r.return_size = sizeof(int);

    return r;
}

int main() 
{
    bool procedure_registered = register_procedure( "addtwo", 2, add );
    bool procedure_registered_again_again = register_procedure("multfive", 5, multiplyFive);
    bool procedure_registered_again = register_procedure( "multtwo", 2, multiply);

    if (!procedure_registered)
    {
        printf("Could not register procedure!\n");
    }

    launch_server();

    /* should never get here, because
       launch_server(); runs forever. */

    return 0;
}
