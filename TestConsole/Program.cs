using System;
using System.Collections.Generic;
using WindowsUtils;
using Wrapper;

namespace TestExec
{
    class Program
    {
        static void Main(string[] args)
        {
            List<Managed.wSessionEnumOutput> thing = Utilities.GetComputerSession(null, false, true);
            Console.WriteLine("###################################################");
            for (int i = 0; i < thing.Count; i++)
            {
                Console.WriteLine("User name: " + thing[i].UserName);
                Console.WriteLine("Session name: " + thing[i].SessionName);
                Console.WriteLine("Session state: " + thing[i].SessionState);
                Console.WriteLine("###################################################");
            }
            Console.WriteLine("\n");
            thing = Utilities.GetComputerSession();
            Console.WriteLine("###################################################");
            for (int i = 0; i < thing.Count; i++)
            {
                Console.WriteLine("User name: " + thing[i].UserName);
                Console.WriteLine("Session name: " + thing[i].SessionName);
                Console.WriteLine("Session state: " + thing[i].SessionState);
                Console.WriteLine("###################################################");
            }
        }
    }
}