#pragma once
#include <chrono>
using namespace System;
using namespace System::Collections::Generic;

namespace Asio {

	ref class Service;

	template<class CLOCK> class TimerImpl;
	public ref class Timer : IObservable<TimeSpan>
	{
		List<IObserver<TimeSpan>^> ^m_observers;
		Queue<IObserver<TimeSpan>^> ^m_subscribeQueue;
		Queue<IObserver<TimeSpan>^> ^m_unsubscribeQueue;
		ref class Unsbscriber
		{
			Timer^ m_timer;
			IObserver<TimeSpan>^ m_observer;

		public:
			Unsbscriber(Timer^ timer, IObserver<TimeSpan> ^observer)
				: m_timer(timer), m_observer(observer)
			{
			}

			~Unsbscriber()
			{
				this->!Unsbscriber();
			}

			!Unsbscriber()
			{
				m_timer->Unsubscribe(m_observer);
			}
		};

	public:
		Timer(Service ^service);
		~Timer();
		!Timer();
		void Start(TimeSpan interval);
		void Stop();
		virtual IDisposable^ Subscribe(IObserver<TimeSpan> ^observer);

	internal:
		TimerImpl<std::chrono::steady_clock> *ptr;
		void Unsubscribe(IObserver<TimeSpan> ^observer);
		void RaiseError(String ^message);
		void Next(TimeSpan elapsed);
		void Completed();

	private:
		void UpdateObservers();
	};

}
