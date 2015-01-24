#include "stdafx.h"
#include "Timer.h"
#include "Service.h"
#include <memory>
#include <msclr/gcroot.h>
#include <boost/asio.hpp>

using namespace System::Threading;

namespace Asio {

	template<class CLOCK>
	class TimerImpl
	{
		boost::asio::basic_waitable_timer<CLOCK> m_timer;
		typename CLOCK::time_point m_start;
		volatile bool m_stop;

	public:
		TimerImpl(boost::asio::io_service &service)
			: m_timer(service), m_stop(true)
		{
		}

		~TimerImpl()
		{
			Stop();
		}

		void Start(msclr::gcroot<Timer^> refClass, TimeSpan span)
		{
			m_stop = false;

			double totalMilliSeconds = span.TotalMilliseconds;
			auto interval = std::chrono::milliseconds((int)totalMilliSeconds);
			m_start = CLOCK::now();
			m_timer.expires_from_now(interval);
			Start(refClass, interval);
		}

		void Start(msclr::gcroot<Timer^> refClass, const std::chrono::microseconds &interval)
		{
			auto self = this;
			auto onTimer = [self, refClass, interval](const boost::system::error_code& ec)
			{
				if (ec) {
					// error;
					refClass->RaiseError(gcnew String(ec.message().c_str()));
					return;
				}

				auto expired = self->m_timer.expires_at();
				auto elapsed = expired - self->m_start;
#if 0
				auto tick = std::chrono::duration_cast <
					std::chrono::duration < int64_t, std::ratio<INTMAX_C(1), INTMAX_C(10000000)> >
				> (elapsed);
				refClass->Next(TimeSpan(tick.count()));
#else
				auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
				refClass->Next(TimeSpan(0, 0, 0, 0, milliseconds.count()));
#endif

				// next
				if (!self->m_stop){
					auto from = self->m_timer.expires_at();
					self->m_timer.expires_at(from + interval);

					self->Start(refClass, interval);
				}
			};
			m_timer.async_wait(onTimer);
		}

		void Stop()
		{
			m_stop = true;
			m_timer.cancel();
		}
	};


	Timer::Timer(Service ^service)
		: ptr(new TimerImpl<std::chrono::steady_clock>(*service->ptr))
	{
		m_observers = gcnew List<IObserver<TimeSpan>^>();
		m_subscribeQueue = gcnew Queue<IObserver<TimeSpan>^>();
		m_unsubscribeQueue = gcnew Queue<IObserver<TimeSpan>^>();
	}

	Timer::~Timer()
	{
		this->!Timer();
	}

	Timer::!Timer()
	{
		delete ptr;
	}

	void Timer::Start(TimeSpan interval)
	{
		ptr->Start(this, interval);
	}

	void Timer::Stop()
	{
		ptr->Stop();
	}

	IDisposable^ Timer::Subscribe(IObserver<TimeSpan> ^observer)
	{
		auto obj = ((System::Collections::ICollection^)m_subscribeQueue)->SyncRoot;
		Monitor::Enter(obj);
		try
		{
			m_subscribeQueue->Enqueue(observer);
		}
		finally
		{
			Monitor::Exit(obj);
		}

		return gcnew Unsbscriber(this, observer);
	}

	void Timer::Unsubscribe(IObserver<TimeSpan> ^observer)
	{
		auto obj = ((System::Collections::ICollection^)m_unsubscribeQueue)->SyncRoot;
		Monitor::Enter(obj);
		try
		{
			m_unsubscribeQueue->Enqueue(observer);
		}
		finally
		{
			Monitor::Exit(obj);
		}
	}

	void Timer::UpdateObservers()
	{
		// add
		{
			auto obj = ((System::Collections::ICollection^)m_subscribeQueue)->SyncRoot;
			Monitor::Enter(obj);
			try
			{
				while (m_subscribeQueue->Count > 0)
				{
					auto observer = m_subscribeQueue->Dequeue();
					m_observers->Add(observer);
				}
			}
			finally
			{
				Monitor::Exit(obj);
			}
		}

		// remove
		{
			auto obj = ((System::Collections::ICollection^)m_unsubscribeQueue)->SyncRoot;
			Monitor::Enter(obj);
			try
			{
				while (m_unsubscribeQueue->Count > 0){
					auto observer = m_unsubscribeQueue->Dequeue();
					m_observers->Remove(observer);
				}
			}
			finally
			{
				Monitor::Exit(obj);
			}
		}
	}

	void Timer::RaiseError(String ^message)
	{
		UpdateObservers();

		for each(auto o in m_observers)
		{
			o->OnError(gcnew System::Exception(message));
		}
		m_observers->Clear();
	}

	void Timer::Next(TimeSpan elapsed)
	{
		UpdateObservers();

		for each(auto o in m_observers)
		{
			o->OnNext(elapsed);
		}
	}

	void Timer::Completed()
	{
		UpdateObservers();

		for each(auto o in m_observers)
		{
			o->OnCompleted();
		}
		m_observers->Clear();
	}

} // namespace
