#ifndef _LIBNET_ERROR_H_
#define _LIBNET_ERROR_H_ 

enum e_libnet_error
{
	e_libnet_err_noerror = 0,								//successful

	//common error
	e_libnet_err_error = 0x01000001,						//failed
	e_libnet_err_invalidhandle,								//invalid NETHANDLE
	e_libnet_err_invalidparam,								//invalid parameters
	e_libnet_err_methodnotimp,								//method is not implemented

	//library error
	e_libnet_err_noninit = 0x02000001,						//library is not initialized
	e_libnet_err_nonioc,									//have no io_context
	e_libnet_err_stillusing,								//library is in using

	//server error
	e_libnet_err_srvcreate = 0x03000001,					//failed to create server 
	e_libnet_err_srvstart,									//failed to start server
	e_libnet_err_srvmanage,									//failed to manage server
	e_libnet_err_srvlistensocknotopen,						//listen-socket is not open
	e_libnet_err_srvlistensocksetopt,						//failed to set listen-socket option
	e_libnet_err_srvlistensockbind,							//failed to bind listen-addr
	e_libnet_err_srvinvalidip,								//invalid ip address(server)
	e_libnet_err_srvlistenstart,							//failed to listen listen-socket
	e_libnet_err_srvcreatecli,								//failed to create client(server)
	e_libnet_err_srvmanagercli,								//failed to manager client(server)

	//client error
	e_libnet_err_clicreate = 0x04000001,					//failed to create client(client)
	e_libnet_err_climanage,									//failed to manager client
	e_libnet_err_clisocknotopen,							//client socket is not open
	e_libnet_err_cliopensock,								//failed to open socket
	e_libnet_err_cliinvalidip,								//invalid ip address(client)
	e_libnet_err_clibind,									//failed to bind addr
	e_libnet_err_clisetsockopt,								//failed to set socket option
	e_libnet_err_cliconntimeout,							//timeout connecting server
	e_libnet_err_cliconnect,								//failed to connect server
	e_libnet_err_cliinitswritebuff,							//failed to initialize send-buffer
	e_libnet_err_cliwritebufffull,							//send-buffer is full
	e_libnet_err_clireadbuffnull,							//read-buffer is NULL
	e_libnet_err_climanualread,								//manual-recv mode
	e_libnet_err_cliwritedata,								//failed to send data
	e_libnet_err_clireaddata,								//failed to receive data
	e_libnet_err_cliprereadnotfinish,						//last receive operation is not finished
	e_libnet_err_cliautoread,								//automatic-recv mode
	e_libnet_err_clijoinmulticastgroup,						//failed to join multicast group
	e_libnet_err_clileavemulticastgroup,					//failed to leave multicast group
	e_libnet_err_clisetmulticastopt,						//failed to set multicast option
	e_libnet_err_cliunsupportmulticastopt,					//unsupported multicast operation

};

#endif