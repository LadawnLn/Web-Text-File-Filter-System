#include <iostream>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <fstream>  
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread/mutex.hpp>

int parseHttp(char **buf,int *bufsize,char ** const bufRes, std::string keyword, std::string host);
int parseRequest(char * buf, int bufsize, const char * bufDel, char flag);
void html(char *buf, int bufsize, char *keyword,int keysize);
char shared_buf[1024];

namespace tcp_proxy
{
    namespace ip = boost::asio::ip;
	/* bridge类需要将当前类名bridge当做参数传给成员函数时，要继承enable_shared_from_this类 */
    class bridge : public boost::enable_shared_from_this<bridge> 
    {
    public:

        typedef ip::tcp::socket socket_type;
        typedef boost::shared_ptr<bridge> ptr_type;

        bridge(boost::asio::io_service& ios)
            : downstream_socket_(ios),
              upstream_socket_  (ios)
        {
            HttpBuf = NULL;
            HttpBufLen = 0;
            DebugCount = 1;
            HttpBufOffset = NULL;
        }

        socket_type& downstream_socket()
        {
            // Client socket
            return downstream_socket_;
        }

        socket_type& upstream_socket()
        {
            // Remote server socket
            return upstream_socket_;
        }

        /*
            调用upstream_socket_的异步connect方法连接到server端；
            注册handle_upstream_connect完成方法。
        */
        void start(const std::string& upstream_host, unsigned short upstream_port)
        {
            // endpoint表示一个Clint或Server，endpoint(host,port)将端点的IP和端口号绑定。
            // boost::bind是函数适配器，将handle_upstream_connect函数与两个参数配接在一起。
			// upstream_socket_是Proxy与远端服务器之间的socket，async_connect方法异步地与远端服务器建立连接，并执行了handle_upstream_connect函数。
            upstream_socket_.async_connect(
                ip::tcp::endpoint(boost::asio::ip::address::from_string(upstream_host), upstream_port),
                boost::bind(&bridge::handle_upstream_connect,shared_from_this(),boost::asio::placeholders::error));
        }

        /*
            调用upstream_socket_的异步读操作，处理client端过来的数据；
            调用downstream_socket_的异步读操作，处理server端过来的数据。
        */
        void handle_upstream_connect(const boost::system::error_code& error)
        {
            if (!error)
            {
				/*
					读Server传来的数据存入缓存区buffer，并执行了handle_upstream_read函数将buffer中数据发送给Client；
					读Client传来的数据存入缓存区buffer，并执行了handle_downstream_read函数将buffer中数据发送给Server。
				*/
                upstream_socket_.async_read_some(
                    boost::asio::buffer(upstream_data_,max_data_length),
                    boost::bind(&bridge::handle_upstream_read,
                                shared_from_this(),  //shared_from_this指向当前的类对象，即bridge类对象自身。
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred));

                downstream_socket_.async_read_some(
                    boost::asio::buffer(downstream_data_,max_data_length),
                    boost::bind(&bridge::handle_downstream_read,
                                shared_from_this(),
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred));
            }
            else
                close();
        }

    private:

		/* 
		   1. 将从Server异步读的数据，调用dealHttpResponse函数进行处理；
		   2. 之后调用async_write异步地将数据转发到Client；
		   3. 转发完成后调用handle_downstream_write函数，继续从Server读数据。
		*/
        void handle_upstream_read(const boost::system::error_code& error, const size_t& bytes_transferred)
        {

            if (!error)
            {
                //HttpBufLen为0时，说明上次接收数据后ret为 1 || 3 || -1，即HttpBuf数据已发送，此时将HttpBuf释放，并存入新接收到的数据
                if(HttpBufLen == 0)
                {
					//释放老的HttpBuf空间
                    if(HttpBuf != NULL)
                    {
                        delete[] HttpBuf;
                        HttpBuf = NULL;
                    }
                    //构造新的HttpBuf，将当前接收的数据拷贝到其中
                    HttpBuf = new char[bytes_transferred];
                    HttpBufLen = bytes_transferred;
                    memcpy(HttpBuf, upstream_data_, bytes_transferred);
                }
                else              //上次接收数据后ret为2或0，即HttpBuf中数据没有发送或部分没有发送。
                {
                    /*
                        两种情况：
                        1. 如果HttpBufOffset  > HttpBuf，则上次ret=2。
                        2. 如果HttpBufOffset == HttpBuf, 则上次ret=0。
                        两种情况的处理方式都为：
                        1. 申请一个更大的空间，将HttpBuf中所有内容拷贝入新空间中，成为新的HttpBuf。
                        2. 释放老的空间。
                    */
                    char *TempBuf = new char[bytes_transferred + HttpBufLen];
                    memcpy(TempBuf, HttpBufOffset, HttpBufLen);
                    memcpy(TempBuf + HttpBufLen, upstream_data_, bytes_transferred);
                    delete[] HttpBuf;
                    HttpBuf = NULL;
                    HttpBuf = TempBuf;
                    HttpBufLen += bytes_transferred;
                }
                           
                int ret;
                try{
                    ret = dealHttpResponse(&HttpBuf, &HttpBufLen, &HttpBufOffset);
                }   
                catch(std::exception& e)
                {
                    std::cerr << "dealHttpResponse error: " << e.what() << std::endl;
                    ret = -1;
                }   
                switch (ret){
                    case 1:
                    case 3:{
                        async_write(downstream_socket_,
                                boost::asio::buffer(HttpBuf,HttpBufLen),
                                boost::bind(&bridge::handle_downstream_write, 
                                            shared_from_this(), 
                                            boost::asio::placeholders::error));
                        HttpBufLen = 0;
                        break;
                    }
                    case 2:{
                        int dealBufLen = HttpBufOffset - HttpBuf;
                        if(dealBufLen >= HttpBufLen || dealBufLen <= 0)
                        {
                            std::cout << "error when ret = 2\n";
                            close();
                            return;
                        }
                        async_write(downstream_socket_,
                                    boost::asio::buffer(HttpBuf, dealBufLen),
                                    boost::bind(&bridge::handle_downstream_write, 
                                                shared_from_this(), 
                                                boost::asio::placeholders::error));
                        HttpBufLen -= dealBufLen;
                        break; 
                    }
                    case 0:{
                        boost::system::error_code ec;
                        handle_downstream_write(ec);
                        break;
                    }
                    default:{        // -1
                        HttpBufLen = 0;
                        boost::system::error_code ec;
                        handle_downstream_write(ec);
                        break; 
                    }
                }
            }
            else
                close();
        }

        /*
			1. 从Server异步读数据，并存入buffer中。
		    2. 之后调用handle_upstream_read函数将buffer中的数据转发到Client。
        */
		void handle_downstream_write(const boost::system::error_code& error)
        {
            if (!error)
            {
                upstream_socket_.async_read_some(
                    boost::asio::buffer(upstream_data_,max_data_length),
                    boost::bind(&bridge::handle_upstream_read,
                                shared_from_this(),
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred));
            }
            else
                close();
        }


		/* 
		   1. 将从Client异步读的数据，调用dealHttpRequest函数进行处理；
		   2. 之后调用async_write异步地将数据转发到Server；
		   3. 转发完成后调用handle_upstream_write函数，继续从Client读数据。
		*/
		void handle_downstream_read(const boost::system::error_code& error,
                                    const size_t& bytes_transferred)
        {
            if (!error)
            {
                dealHttpRequest(downstream_data_, bytes_transferred);
                async_write(upstream_socket_,
                            boost::asio::buffer(downstream_data_,bytes_transferred),
                            boost::bind(&bridge::handle_upstream_write,
                                        shared_from_this(),
                                        boost::asio::placeholders::error));
            }
            else
                close();
        }

		/* 
		   1. 从Client异步读数据，存入buffer中；
		   2. 之后调用handle_downstream_read函数将buffer中的数据转发到Server。
        */
		void handle_upstream_write(const boost::system::error_code& error)
        {
            if (!error)
            {
                downstream_socket_.async_read_some(
                    boost::asio::buffer(downstream_data_,max_data_length),
                    boost::bind(&bridge::handle_downstream_read,
                                shared_from_this(),
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred));
            }
            else
                close();
        }

        void close()
        {
            if(HttpBuf != NULL)
            {
                delete[] HttpBuf;
                HttpBuf = NULL;
            }
            boost::mutex::scoped_lock lock(mutex_);

            if (downstream_socket_.is_open())
            {
                downstream_socket_.close();
            }

            if (upstream_socket_.is_open())
            {
                upstream_socket_.close();
            }
        }

     
        /*******************************************************************************
        return:
            0，表示http数据不完整，此时应该继续等待下一个tcp数据包；
            (1)||(3)，表示数据已经处理完了，此时应该立刻将全部数据转发给client；
            -1，表示数据首部信息无法处理，此时直接发送，不进行处理；
            2，表示数据部分处理完了，此时将部分数据发给client，未处理部分交给handle_downstream_write处理。
        *HttpBufLen: 代表当前HttpBuf动态空间中还未被发送的数据大小。
        *HttpBufOffset：代表还未处理的数据包位置。
            return=0 时等于HttpBuf;
            return=1||3 时等于NULL;
            return=2 时指向buf数据中的某个字节。
            return=-1 时指向NULL。

        dealHttpResponse函数结束后内存示意图：                    
        HttpBuf     *HttpBufOffset
           +-------------------------------------+
           |             |      HttpBufLen       | 
           +-------------------------------------+
        **********************************************************************************/
        int dealHttpResponse(char **HttpBuf, int *HttpBufLen, char ** HttpBufOffset)
        {
            std::string s = downstream_socket_.remote_endpoint().address().to_string();
            std::string KeyWord(shared_buf);
            return parseHttp(HttpBuf, HttpBufLen, HttpBufOffset, KeyWord, s);//DealPdf(buf,bufsize);
        }

        int dealHttpRequest(char *buf, int bufsize)
        {
			parseRequest(buf, bufsize, "Accept-Encoding:",1);
			parseRequest(buf, bufsize, "Range:",2);
            return 0;
        }
        socket_type downstream_socket_;
        socket_type upstream_socket_;
			
        enum { max_data_length = 8192 };  // 缓存区长度设置为8KB
        char downstream_data_[max_data_length];
        char upstream_data_  [max_data_length];
        char *HttpBuf;                    // 从server收到的数据，需要交给dealHttp进行处理
        int HttpBufLen;                   // HttpBuf中未发送的数据长度
        char *HttpBufOffset;              // HttpBufOffset之前的buf已被处理，之后的还未处理
        int DebugCount;
        boost::mutex mutex_;              // 同步锁，在关闭连接时处理用


    public:

        class acceptor
        {
        public:

            acceptor(boost::asio::io_service& io_service,
                     const std::string& local_host, unsigned short local_port,
                     const std::string& upstream_host, unsigned short upstream_port)
                : io_service_(io_service),
                  localhost_address(boost::asio::ip::address_v4::from_string(local_host)),
                  acceptor_(io_service_,ip::tcp::endpoint(localhost_address,local_port)),
                  upstream_port_(upstream_port),
                  upstream_host_(upstream_host),
                  keyword_socket_(io_service)
            {}

            /*
                提前构造一个bridge用于下一个来自downstream的连接请求；
                调用acceptor_的异步accept方法，注册完成处理函数handle_accept；
            */
            bool accept_connections()
            {
                try
                {
                    session_ = boost::shared_ptr<bridge>(new bridge(io_service_));
                    acceptor_.async_accept(session_->downstream_socket(), boost::bind(&acceptor::handle_accept, this, 
                    boost::asio::placeholders::error));
                }
                catch(std::exception& e)
                {
                    std::cerr << "acceptor exception: " << e.what() << std::endl;
                    return false;
                }

                return true;
            }

            /*
                这个acceptor实例主要用来和python进程进行沟通，专门用来接收python写的界面传过来的敏感词，因此不需要构建bridge。
            */
            bool accept_keyword_read()
            {
                try
                {
                    acceptor_.async_accept(
                    keyword_socket_, 
                    boost::bind(&bridge::acceptor::handle_keyword_accept,
                                this,
                                boost::asio::placeholders::error));
                }
                catch(std::exception& e)
                {
                    std::cerr << "acceptor exception: " << e.what() << std::endl;
                    return false;
                }

                return true;
            }
        private:

            /*
                此时，session中存储这当前连接的bridge实例，其中downstream_socket_已经有值；
                调用session中的start函数来建立upstream_socket_，以此连接server端；
                调用accept_connections函数为下一个session做准备。
            */
            void handle_accept(const boost::system::error_code& error)
            {

                if (!error)
                {
                    session_->start(upstream_host_,upstream_port_);

                    if (!accept_connections())
                    {
                        std::cerr << "Failure during call to accept." << std::endl;
                    }
                }
                else
                {
                    std::cerr << "Error: " << error.message() << std::endl;
                }
            }

            void handle_keyword_accept(const boost::system::error_code& error)
            {
                printf("keyword connect success\n");
                if(!error)
                {    
                    keyword_socket_.async_read_some(
                    boost::asio::buffer(shared_buf, max_data_length),
                    boost::bind(&bridge::acceptor::handle_keyword_read,
                                this,    
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred));                                   
                }
                else
                {
                    std::cerr << "Error: " << error.message() << std::endl;
                }
                    
            }

            void handle_keyword_read(const boost::system::error_code& error, const size_t& bytes_transferred)
            {
                
                if(!error)
                {  
                    for(int i = 0; i < bytes_transferred; i++)
                        printf("%X ",shared_buf[i]);
                    std::cout<<std::endl;
                    shared_buf[bytes_transferred - 2] = 0;
                    keyword_socket_.async_read_some(
                    boost::asio::buffer(shared_buf, max_data_length),
                    boost::bind(&bridge::acceptor::handle_keyword_read,
                                this,
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred)); 
                }
                else
                {
                    std::cerr << "Error: " << error.message() << std::endl;
                }
            }
            
            enum { max_data_length = 8192 }; 	   // 缓存区长度设置为8KB
            boost::asio::io_service& io_service_;
            ip::address_v4 localhost_address;
            ip::tcp::acceptor acceptor_;           // 接收器，循环处理不同的client请求，每一个client对应一个bridge
            ptr_type session_;                     // 指向acceptor_当前正在处理bridge
            unsigned short upstream_port_;         // server端端口号
            std::string upstream_host_;            // server端ip地址
            socket_type keyword_socket_;           // 用来接收界面传来的keyword   
            char keyword_stream_[max_data_length];  
        };

    };
}

int main(int argc, char* argv[])
{
	if (argc != 3)
    {
        std::cerr << "usage: tcpproxy_server <local host ip> <local port> " << std::endl;
        return 1;
    }

	const std::string local_host    = argv[1];
	const unsigned short local_port = static_cast<unsigned short>(::atoi(argv[2]));
    
    std::ifstream AugInput;
    AugInput.open("ServerHost");
    if(AugInput.fail())
    {
        std::cerr << "ServerHost does not exist";
        return 1;
    }
    unsigned short remote_port;
    std::string remote_host;
    std::vector<unsigned short> PortVector;
    std::vector<std::string> HostVector;
    while(true)
    {
        AugInput >> remote_host; 
        if(AugInput.eof())break;
        std::cout << remote_host << " " << 80 << std::endl;
        PortVector.push_back(80);
        HostVector.push_back(remote_host);
    }
    AugInput.close();
    
    try
    {
		boost::asio::io_service ios;
        /* 单独建立一个acceptor来处理界面传过来的keyword, 端口号设置为4999 */
        boost::shared_ptr<tcp_proxy::bridge::acceptor> acceptor_keyword(new tcp_proxy::bridge::acceptor(ios, local_host, 4999, "", 0));
        acceptor_keyword->accept_keyword_read();
        
        /* acceptorVector里面装多个acceptorPoint, 可以同时代理多个远端服务器 */
        std::vector<boost::shared_ptr<tcp_proxy::bridge::acceptor> > acceptorVector;
        for(int i = 0; i < HostVector.size(); i++)
        {
            boost::shared_ptr<tcp_proxy::bridge::acceptor> acceptorPoint(new tcp_proxy::bridge::acceptor(ios, local_host, 5500+i, HostVector[i], PortVector[i]));    
            acceptorPoint->accept_connections();
            acceptorVector.push_back(acceptorPoint);
        }
        ios.run();
    }
    catch(std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

