
/*== MAIN :

	sFD = getSock()

	FD_ZERO( &aFDs )
	bzero( &servA, servS )

	servA... * 3

  if bind( sFD, ( const * )&servA, servS ) :					 err() !
  if listen( sFD, SOMAXCONN ) :												 err() !

=	==== WHILE :
		rFDs = wFDs = aFDs
		if select( mFD + 1, &rFDs, &wFDs, 0, 0 ) < 0 :		 err() !

		for cFD = 0 to mFD :
			if !FD_ISSET( cFD, &rFDs ) :									continue !

=	=	=	==== IF NEW :
				nFD = accept( sFD, ( * )&servA, ( * )&servS )

				if nFD >= 0 :
					addCli( nFD )
					break

=	=	=	==== IF OLD :
				bytC = recv( cFD, rbuf, 1000, 0 )

				if bytC > 0 :
					rbuf[ bytC ] = 0
					mbuf[ cFD ] = str_join( mbuf[ cFD ], rbuf )
					usrMsg( cFD )

				else :												 delCli( cFD ) & break !

==== getSock()
	mFD = socket( AF_INET, SOCK_STREAM, 0 )
	if mFD < 0 :																				 err() !
	FD_SET( mFD, &aFDs )

==== SYSMSG( msg, authFD )
	for fd = 0 to mFD :
		if FD_ISSET( fd, &wFDs ) && fd != authFD
			send( fd, msg, strlen( msg ), 0 )

==== USRMSG( fd )
	while extract_message( &( mbuf[ fd ]), &msg )
		sysMsg( wbuf, fd )
		sysMsg( msg, fd )
		free( msg )

==== ADDCLI( fd )
	if fd > mFD :																			mFD = fd !
	ids[ fd ] = cliC++
	mbuf[ fd ] = 0
	FD_SET( fd, &aFDs )

	sysMsg()

==== DELCLI( fd )
	sysMsg()

	free( mbuf[ fd ] )
	FD_CLR( fd, &aFDs )
	close( fd )

==== ERR()
	write()
*/


#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>


int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

int			cliC = 0, mFD = 0, ids[ 65535 ];
char		rbuf[ 1001 ], wbuf[ 42 ], *mbuf[ 65535 ];
fd_set	rFDs, wFDs, aFDs;

void	err()
{
	write( 2, "Fatal error\n", 12 );
	exit( 1 );
}

int		getSock()
{
	mFD = socket( AF_INET, SOCK_STREAM, 0 );
	if( mFD < 0 )
		err();
	FD_SET( mFD, &aFDs );
	return( mFD );
}

void	sysMsg( char *msg, int auth )
{
	for ( int fd = 0; fd <= mFD; fd++ )
	{
		if ( FD_ISSET( fd, &wFDs ) && fd != auth )
			send( fd, msg, strlen( msg ), 0 );
	}
}

void	usrMsg( int fd )
{
	char	*msg;

	while( extract_message( &( mbuf[ fd ]), &msg ))
	{
		sprintf( wbuf, "client %d: ", ids[ fd ]);
		sysMsg( wbuf, fd );
		sysMsg( msg, fd );
		free( msg );
	}
}

void	addCli( int fd )
{
	if ( fd > mFD )
		mFD = fd;
	ids[ fd ] = cliC++;
	mbuf[ fd ] = 0;
	FD_SET( fd, &aFDs );

	sprintf( wbuf, "server: client %d just arrived\n", ids[ fd ]);
	sysMsg( wbuf, fd );
}

void	delCli( int fd )
{
	sprintf( wbuf, "server: client %d just left\n", ids[ fd ]);
	sysMsg( wbuf, fd );

	free( mbuf[ fd ] );
	FD_CLR( fd, &aFDs );
	close( fd );
}

int main(int ac, char **av )
{
	if( ac != 2 )
	{
		write( 2, "Wrong number of arguments\n", 26 );
		exit( 1 );
	}

	FD_ZERO( &aFDs );
	int sFD = getSock();

	struct sockaddr_in servA;
	socklen_t servS = sizeof( servA );
	bzero( &servA, servS );

	servA.sin_family = AF_INET;
	servA.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servA.sin_port = htons( atoi( av[ 1 ]));

	if( bind( sFD, ( const struct sockaddr * )&servA, servS ) != 0 )
		err();

	if( listen( sFD, SOMAXCONN ) != 0 )
		err();

	while( 1 )
	{
		rFDs = wFDs = aFDs;

		int slct = select( mFD + 1, &rFDs, &wFDs, 0, 0 );

		if ( slct < 0 )  err();
		if ( slct == 0 ) continue;

		for( int cFD = 0; cFD <= mFD; cFD++ )
		{
			if( !FD_ISSET( cFD, &rFDs ))
				continue;

			if( cFD == sFD )
			{
				int nFD = accept( sFD, ( struct sockaddr * )&servA, ( socklen_t * )&servS );

				if( nFD >= 0 )
				{
					addCli( nFD );
					break;
				}
			}

			else
			{
				int bytC = recv( cFD, rbuf, 1000, 0 );

				if( bytC <= 0 )
				{
					delCli( cFD );
					break;
				}

				else
				{
					rbuf[ bytC ] = 0;
					mbuf[ cFD ] = str_join( mbuf[ cFD ], rbuf );
					usrMsg( cFD );
				}
			}
		}
	}
}

/*

+ stdio
+ stdlib

ac...
FD_ZERO( aFDs )
sFD = getSock()
bzero()
sin_...

if bind() : err()
if listen() : err()

while true {
	rFDs = wFDs = aFDs
	if select( mFD + 1... ) : err()

	for ... {
		FD_ISSET( cFD rFDs )

		if cfFD = sFD {
			nFD = accept( sFD... )
			if n > 0 : addCli() + break;
		}

		else {
			bytC = rev( cFD, rbuf ... )

			if bytC ' 0 : dekCli() + break;

			else
			{
				rbuf[ bytC ] = 0
				mbuf[ cFD ] = strjoin( mbuf[], rbuf )
				usrMsg()
			}
		}
	}
}

*/