using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Reactive.Linq;


namespace Sample
{
    class Program
    {
        static void Main(string[] args)
        {
            {
                var service = new Asio.Service();
                var timer = new Asio.Timer(service);

                timer
                    .Take(3)
                    .Subscribe(elapsed =>
                    {
                        Console.WriteLine("Expired: " + elapsed);
                    }
                , () =>
                {
                    Console.WriteLine("Completed");
                    timer.Stop();
                }
                );

                timer.Start(TimeSpan.FromMilliseconds(330));
                service.Run();
                Console.WriteLine("Service ended");
            }

            // thread
            {
                var service = new Asio.Service();
                var timer = new Asio.Timer(service);

                timer
                    .Subscribe(elapsed =>
                    {
                        Console.WriteLine("Expired: " + elapsed);
                    }
                , ex =>
                {
                    Console.WriteLine(ex);
                }
                , () =>
                {
                    Console.WriteLine("Completed");
                }
                );

                Task.Run(() =>
                {
                    System.Threading.Thread.Sleep(1000);
                    timer.Stop();
                });

                timer.Start(TimeSpan.FromMilliseconds(330));
                service.Run();
                Console.WriteLine("Service ended");
            }
        }
    }
}