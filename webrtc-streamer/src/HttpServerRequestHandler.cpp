/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** HttpServerRequestHandler.cpp
** 
** -------------------------------------------------------------------------*/

#include <string.h>
#ifndef WIN32
#include <unistd.h>
#include <dirent.h>
#endif

#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>



#include "rtc_base/logging.h"

#include "HttpServerRequestHandler.h"


int log_message(const struct mg_connection *conn, const char *message) 
{
    fprintf(stderr, "%s\n", message);
    return 0;
}

static struct CivetCallbacks _callbacks;
const struct CivetCallbacks * getCivetCallbacks() 
{
    memset(&_callbacks, 0, sizeof(_callbacks));
    _callbacks.log_message = &log_message;
    return &_callbacks;
}


/* ---------------------------------------------------------------------------
**  Civet HTTP callback 
** -------------------------------------------------------------------------*/
class RequestHandler : public CivetHandler
{
  public:
	RequestHandler(HttpServerRequestHandler::httpFunction & func): m_func(func) {
	}	  
	
    bool handle(CivetServer *server, struct mg_connection *conn)
    {
  

        const struct mg_request_info *req_info = mg_get_request_info(conn);
        
        RTC_LOG(LS_INFO) << "uri:" << req_info->request_uri;
        
		// read input
		Json::Value  in = this->getInputMessage(req_info, conn);
		
		// invoke API implementation
		std::tuple<int, std::map<std::string,std::string>,Json::Value> out(m_func(req_info, in));
		
        int code = std::get<0>(out);
        std::string answer;

		// fill out
        Json::Value& body = std::get<2>(out);
		if (body.isNull() == false)
		{
            if (body.isString()) {
                answer = body.asString();
            } else {
                answer = Json::writeString(m_writerBuilder,body);
            }
		} else {
            code = 500;
            answer = mg_get_response_code_text(conn, code);
        }

        RTC_LOG(LS_VERBOSE) << "code:" << code << " answer:" << answer;	
        mg_printf(conn,"HTTP/1.1 %d OK\r\n", code);
        mg_printf(conn,"Access-Control-Allow-Origin: *\r\n");
        mg_printf(conn,"Content-Type: text/plain\r\n");
        mg_printf(conn,"Content-Length: %zd\r\n", answer.size());
        std::map<std::string,std::string> & headers = std::get<1>(out);
        for (auto & it : headers) {
            mg_printf(conn,"%s: %s\r\n", it.first.c_str(), it.second.c_str());
        } 
        mg_printf(conn,"\r\n");
        mg_write(conn,answer.c_str(),answer.size());
        
        return true;
    }
    bool handleGet(CivetServer *server, struct mg_connection *conn)
    {
        return handle(server, conn);
    }
    bool handlePost(CivetServer *server, struct mg_connection *conn)
    {
        return handle(server, conn);
    }
    bool handlePatch(CivetServer *server, struct mg_connection *conn)
    {
        return handle(server, conn);
    }
    bool handleDelete(CivetServer *server, struct mg_connection *conn)
    {
        return handle(server, conn);
    }
    
  private:
    HttpServerRequestHandler::httpFunction      m_func;	
    Json::StreamWriterBuilder                   m_writerBuilder;
    Json::CharReaderBuilder                     m_readerBuilder;
  
    Json::Value getInputMessage(const struct mg_request_info *req_info, struct mg_connection *conn) {
        Json::Value  jmessage;

        // read input
        long long tlen = req_info->content_length;
        if (tlen > 0)
        {
            std::string body = CivetServer::getPostData(conn);

            // parse in
            std::unique_ptr<Json::CharReader> reader(m_readerBuilder.newCharReader());
            std::string errors;
            if (!reader->parse(body.c_str(), body.c_str() + body.size(), &jmessage, &errors))
            {
                RTC_LOG(LS_WARNING) << "Received unknown message:" << body << " errors:" << errors;
                jmessage = body;
            }
        }
        return jmessage;
    }	
};



class WebsocketHandler: public CivetWebSocketHandler {	
	public:
		WebsocketHandler(std::map<std::string,HttpServerRequestHandler::httpFunction> & func): m_func(func) {
		}
				
	private:
		std::map<std::string,HttpServerRequestHandler::httpFunction>      m_func;		
		Json::StreamWriterBuilder                   m_jsonWriterBuilder;
	
		virtual bool handleConnection(CivetServer *server, const struct mg_connection *conn) {
			RTC_LOG(LS_INFO) << "WS connected";
			return true;
		}

		virtual void handleReadyState(CivetServer *server, struct mg_connection *conn) {
			RTC_LOG(LS_INFO) << "WS ready";
		}

		virtual bool handleData(CivetServer *server,
					struct mg_connection *conn,
					int bits,
					char *data,
					size_t data_len) {
			int opcode = bits&0xf;
			printf("WS got %lu bytes\n", (long unsigned)data_len);
						
			if (opcode == MG_WEBSOCKET_OPCODE_TEXT) {
				// parse in
				std::string body(data, data_len);
				Json::CharReaderBuilder builder;
				std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
				Json::Value in;
				if (!reader->parse(body.c_str(), body.c_str() + body.size(), &in, NULL))
				{
					RTC_LOG(LS_WARNING) << "Received unknown message:" << body;
				}
                std::cout << Json::writeString(m_jsonWriterBuilder,in) << std::endl;

                std::string request = in.get("request","").asString();
                auto it = m_func.find(request);

                std::string answer;
                if (it != m_func.end()) {
                    HttpServerRequestHandler::httpFunction func = it->second;
                            
                    // invoke API implementation
                    const struct mg_request_info *req_info = mg_get_request_info(conn);
                    std::tuple<int, std::map<std::string,std::string>,Json::Value> out(func(req_info, in.get("body","")));
                    
                    answer = Json::writeString(m_jsonWriterBuilder,std::get<2>(out));
                } else {
                    answer = mg_get_response_code_text(conn, 500);
                }

				mg_websocket_write(conn, MG_WEBSOCKET_OPCODE_TEXT, answer.c_str(), answer.size());
			}
			
			return true;
		}

		virtual void handleClose(CivetServer *server, const struct mg_connection *conn) {
			RTC_LOG(LS_INFO) << "WS closed";	
		}
		
};

/* ---------------------------------------------------------------------------
**  Constructor
** -------------------------------------------------------------------------*/
HttpServerRequestHandler::HttpServerRequestHandler(std::map<std::string,httpFunction>& func, const std::vector<std::string>& options) 
    : CivetServer(options, getCivetCallbacks())
{

    // register handlers
    for (auto it : func) { 
        CivetHandler* handler = new RequestHandler(it.second);
        this->addHandler(it.first, handler);
        m_handlers.push_back(handler);
    } 	


    this->addWebSocketHandler("/ws", new WebsocketHandler(func));    
}	

/* ---------------------------------------------------------------------------
**  Constructor
** -------------------------------------------------------------------------*/
HttpServerRequestHandler::~HttpServerRequestHandler()
{
    while (m_handlers.size() > 0) {
        CivetHandler* handler = m_handlers.back();
        m_handlers.pop_back();
        delete handler;
    }
}   


