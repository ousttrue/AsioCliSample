#pragma once
#include <boost/asio.hpp>

namespace Asio {

	public ref class Service
	{
	public:
		Service()
			: ptr(new boost::asio::io_service)
		{}

		~Service()
		{
			this->!Service();
		}

		!Service()
		{
			delete ptr;
		}

		void Run()
		{
			ptr->run();
		}

	internal:
		boost::asio::io_service *ptr;
	};

}
